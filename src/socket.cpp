#include "socket.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Socket::Socket(const SocketFamily sf, const SocketType st, const SocketProtocol sp)
{
    // Initializes the WinSock object if it's necessary. WinSock is an singleton object, so it's safe to
    // call WinSocks Create member function more than once.
    WinSock::Create();

    // Initializes the socket parameters.
    mSocketInfo.socketFamily = sf;
    mSocketInfo.socketType = st;
    mSocketInfo.socketProtocol = sp;
    mSendBufferLength = DEFAULT_BUFLEN;
    mReceiveBufferLength = DEFAULT_BUFLEN;
    mReceiveTimeout = DEFAULT_RECEIVE_TIMEOUT;

    // Here we create the actual socket.
    CreateSocket();
    // By default, the socket is in Blocking state.
    SetNonBlockingStatus(false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Socket::~Socket()
{
    // TODO: clean up socket if there is something to clean up.
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::GetReceiveTimeout() const
{
    return mReceiveTimeout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::SetReceiveTimeout(const int mm)
{
    mReceiveTimeout = mm;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int Socket::GetReceiveBufferSize() const
{
    return mReceiveBufferLength;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Socket::IsBlocking() const
{
    return mIsBlocking;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::SetReceiveBufferSize(const unsigned int recSize)
{
    mReceiveBufferLength = recSize;
}



/*
Socket& Socket::operator=(Socket&& other)
{
    if (this != &other)
    {
        Close();
        mSocket = other.mSocket;
        other.mSocket = INVALID_SOCKET;
        return *this;
    }
    return *this;
}
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::Connect(const std::string& address, const int port)
{
     // TODO: create a smart pointer.
     //std::unique_ptr<addrinfo,std::function<void(addrinfo*)>>
     //       pResult(nullptr,[](addrinfo* ar) {freeaddrinfo(ar);});

     // A pointer to addrinfo object(s) (information about all available connections).
     addrinfo* result = nullptr;
     // A pointer to iterate over all results. (see to loop below).
     addrinfo* ptr = nullptr;

     // Here we create a hint which indicates which kind of connection is desired.
     addrinfo hints = CreateHints();

     // We must convert the port to c-string because getaddrinfo function demands it.
     std::string s = std::to_string(port);
     char const *portCString = s.c_str();

    // In this call we get information about the host and which kind of connection is available.
    // The result is a chain of addrinfos in a linked list. Hints filters away wrong kind of connections.
    int status = getaddrinfo(address.c_str(), portCString, &hints, &result);

    /* Something went wrong. */
    if (status != 0)
    {
        // Lets free result before an exception is thrown.
        freeaddrinfo(result);
        throw std::runtime_error("Socket::Connect : " + SocketError::getError(WSAGetLastError()));
    }

    // Flag that indicates if we succeed to create an connection.
    bool connected = false;

    /*
     * Here we try to create a connection in a loop. Remember: the result is an linked list of connection, and
     * we have to try until we make an connection or until the all elements of result are iterated.
     */
    for (ptr = result ; ptr != nullptr ; ptr = ptr->ai_next)
    {
        /* Lets try to make a connection. */
        status = connect( mSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));

        /* We made it! we can stop right here. */
        if (status == 0)
        {
            connected = true;
            break;
        }
    }

    /* Always remember to free the result. */
    freeaddrinfo(result);

    /* No connection. An exception is thrown. */
    if (!connected)
    {
        throw SocketException("Socket::Connect : connection failed.");
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::Receive(std::string& s)
{
    int recBytes = 0;
    char receiveBuffer[mReceiveBufferLength];

    // If receiveTimeout is zero or negative, the operation is considered an infinity operation (see below).
    bool infinity = (mReceiveTimeout <= 0);

    // The result indicates the amount of bytes received or SOCKER_ERROR if something goes wrong.
    int result = 0;
    do
    {
        // Here we check if the socket is ready to read.
        bool ready = true;
        // If receiveTimeout is zero or an negative number, the socket is goin to wait forever to complete its task.
        // Otherwise the socket is going to wait for mReceiveTimeout amount of time to wait and then tell if its ready
        // to read.
        if (!infinity) ready = CheckStatus(mReceiveTimeout, SocketStatus::READ);
        if (!ready) { return recBytes; }

        // A different call for forever waiting operation and timed operation.
        // MSG_WAITALL continues only, if (1) recBuffer is full or (2) the host has closed the connection or
        // (3) if the request has been canceled or an error occurred.
        if (infinity) result = recv(mSocket, receiveBuffer, sizeof(receiveBuffer), MSG_WAITALL);
        else result = recv(mSocket, receiveBuffer, sizeof(receiveBuffer), 0);

        // If the input buffer has something for us, we must append the result to the string buffer 's'.
        if (result > 0) {
            // The receiveBuffer is totally full. We append everything to 's' and continue.
            if (result == sizeof(receiveBuffer))
            {
                s.append(receiveBuffer, 0, result);
                recBytes += result;
                continue;
            }
            // We add '\0' to the buffer so we know where the data ends. Then we append the data to the buffer 's'.
            receiveBuffer[result] = '\0';
            recBytes += result;
            s += receiveBuffer;
            return recBytes;
        }
        // Nothing to read. Lets break the loop.
        else if (result == 0) { break; }
        // An error occurred.
        else if (result == SOCKET_ERROR)
        {
            // Lets get that error.
            int error = WSAGetLastError();

            // WSAEWOULDBLOCK is not an actual error. It only tells that the socket is in non-blocking state and
            // there's nothing more to read in the recBuffer. We can now break the loop and return the data we
            // got so far.
            if (error == WSAEWOULDBLOCK) {
                return recBytes;
            }
            // There was a real error. Throw an exception.
            else throw SocketException("Socket.Receive: " + SocketError::getError(error));
        };
    // The condition of the loop. We stop only when the result == 0, which tells us that there is no more data to read.
    } while (result > 0);

    return recBytes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::ReceiveFrom(std::string& s, EndPoint& remote)
{
    int recBytes = 0;
    char receiveBuffer[mReceiveBufferLength];
    int addr_len = sizeof(struct sockaddr_in);

    /* Result on saatujen tavujen m‰‰r‰, tai SOCKET_ERROR jos tapahtuu virhe. */
    int result = 0;
    do
    {
        /* Tsekataan voidaanko socketista lukea. Odotetaan korkeintaan mReceiveTimeout millisekuntia. */
        bool ready = CheckStatus(mReceiveTimeout, SocketStatus::READ);
        if (!ready) { return recBytes; }

        result = recvfrom(mSocket,
                          receiveBuffer,
                          sizeof(receiveBuffer),
                          0,
                          reinterpret_cast<sockaddr*>(remote()),
                          &addr_len);

        if (result > 0) {
            if (result == sizeof(receiveBuffer))
            {
                s.append(receiveBuffer, 0, result);
                recBytes += result;
                continue;
            }

            receiveBuffer[result] = '\0';
            recBytes += result;
            s += receiveBuffer;
            return recBytes;
        }
        else if (result == 0) { break; }
        else if (result == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                return recBytes;
            }
            throw SocketException("Socket.ReceiveFrom: " + SocketError::getError(error));
        };
    } while (result > 0);
    return recBytes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::Send(const std::string& data)
{
    const char* dataCFormat = data.c_str();
    bool ready = CheckStatus(10, SocketStatus::WRITE);
    if (!ready) { std::cout << "EI VOI KIRJOITTAA" << std::endl; return 0; }

    int result = send(mSocket, dataCFormat, static_cast<int>(strlen(dataCFormat)), 0);

    if (result == SOCKET_ERROR)
    {
        throw SocketException("Socket::Send(). " + SocketError::getError(WSAGetLastError()));
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::SendTo(const std::string& data, EndPoint& remote)
{
    const char* dataCString = data.c_str();
    bool ready = CheckStatus(10, SocketStatus::WRITE);

    // TODO: heit‰ poikkeus?
    if (!ready) { std::cout << "EI VOI KIRJOITTAA" << std::endl; return 0; }

    int result = sendto(mSocket,
                        dataCString,
                        static_cast<int>(strlen(dataCString)),
                        0,
                        reinterpret_cast<sockaddr*>(remote()),
                        sizeof(struct sockaddr_in));

    if (result == SOCKET_ERROR)
    {
        //Close();
        SocketException("Socket::SendTo(). " + SocketError::getError(WSAGetLastError()));
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::Close()
{
    // Jos socket on invalid-tilassa, niin poistutaan.
    if (mSocket == INVALID_SOCKET) return;
    int result = closesocket(mSocket);
    if (result != 0)
    {
        SocketException(SocketError::getError(WSAGetLastError()));
    }
    mSocket = INVALID_SOCKET;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Socket Socket::Accept()
{
    SOCKET result = accept(mSocket, nullptr, nullptr);
    if (result == INVALID_SOCKET)
    {
        SocketException("Socket::Accept. " + SocketError::getError(WSAGetLastError()));
    }
    Socket s(result);
    return s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::CreateSocket()
{
    SOCKET s = socket(static_cast<int>(mSocketInfo.socketFamily),
                      static_cast<int>(mSocketInfo.socketType),
                      static_cast<int>(mSocketInfo.socketProtocol));

    if (s == INVALID_SOCKET) SocketException(SocketError::getError(WSAGetLastError()));
    else mSocket = s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::SetNonBlockingStatus(bool status)
{
    u_long iMode;
    iMode = status;

    //-------------------------
    // Set the socket I/O mode: In this case FIONBIO
    // enables or disables the blocking mode for the
    // socket based on the numerical value of iMode.
    // If iMode = 0, blocking is enabled;
    // If iMode != 0, non-blocking mode is enabled.

    int result = ioctlsocket(mSocket, FIONBIO, &iMode);
    if (result != NO_ERROR) {
        throw SocketException("Socket::SetNonBlockingStatus : " + status);
    }
    mIsBlocking = !status;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::GetAvailableBytes(const unsigned int mm_seconds)
{
    using namespace std::chrono;

    u_long bytesAvailable = 0;
    milliseconds timeToWait(mm_seconds);
    auto t1 = high_resolution_clock::now();

    while (!bytesAvailable && ioctlsocket(mSocket, FIONREAD, &bytesAvailable) >= 0)
    {
        std::this_thread::sleep_for(milliseconds(mm_seconds));
        auto t2 = high_resolution_clock::now();
        milliseconds delta = duration_cast<milliseconds>(t2 - t1);
        if (timeToWait.count()-delta.count() < 0) return bytesAvailable;
    }
    return static_cast<int>(bytesAvailable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Socket::Bind()
{
    addrinfo* result = nullptr;
    addrinfo* ptr = nullptr;

    addrinfo hints = CreateHints();

    hints.ai_flags = AI_PASSIVE;

    /* Muutetaan ip:t sellaiseen muotoon ett‰ getaddrinfo ymm‰rt‰‰. */
    const char* ip = mEndpoint.GetIpAddress();
    if (strcmp(ip, "0.0.0.0") == 0) ip = NULL;
    else if (strcmp(ip, "127.0.0.1") == 0) ip = "localhost";

    int status = getaddrinfo(ip,
                             std::to_string(mEndpoint.GetPortNumber()).c_str(),
                             &hints,
                             &result);

    if (status != 0)
    {
        // TODO: heit‰ poikkeus.
        std::cout << "Socket::Bind() -> getaddrinfo() ep‰onnistui." << std::endl;
        // Error::PrintError(WSAGetLastError());
        return false;
    }

    for (ptr = result ; ptr != nullptr ; ptr = ptr->ai_next)
    {
        mSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (mSocket == INVALID_SOCKET)
        {
            // TODO. Heit‰ poikkeus.
            std::cout << "Socket::Bind() -> socket() ep‰onnistui." << std::endl;
            //Error::PrintError(WSAGetLastError());
            return false;
        }

        status = bind(mSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
        if (status == SOCKET_ERROR)
        {
            //Close();
            continue;
        }
    }

    freeaddrinfo(result);

    if (mSocket == INVALID_SOCKET)
    {
        // TODO: Heit‰ poikkeus.
        std::cout << "Varoitus: Socket::Bind() -> Ei saatu bindattua." << std::endl;
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Socket::CheckStatus(const int timeLimit_msec, const SocketStatus status)
{
    // T‰nne ei p‰‰dyt‰, jos mReceiveTimeout <= 0.
    assert(mReceiveTimeout>0);

    struct timeval tv;
    /* Struct joka pit‰‰ sis‰ll‰‰n socketteja. T‰ss‰ tapauksessa vain t‰m‰ socketti.*/
    fd_set sockets;

    tv.tv_sec = 0;
    tv.tv_usec = timeLimit_msec*1000;

    int result;

    do {
        /* Tyhjennet‰‰n sockets. */
        FD_ZERO(&sockets);
        /* Laitetaan socket sockets:iin. */
        FD_SET(mSocket, &sockets);
        /* Jos halutaan tsekata ett‰ onko socketilla jotain luettavaa... */
        if (status == SocketStatus::READ)
        {
            result = select(mSocket+1, &sockets, NULL, NULL, &tv);
        }

        /* Jos halutaan tsekata voiko socket l‰hett‰‰ dataa... */
        else if (status == SocketStatus::WRITE)
        {
            result = select(mSocket+1, NULL, &sockets, NULL, &tv);
        }
        if (result == 0) return false; // Aikakatkaisu
    }
    while (result == SOCKET_ERROR && errno == EINTR);
    if (result > 0 && FD_ISSET(mSocket, &sockets)) return true;
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

addrinfo Socket::CreateHints() const
{
     addrinfo hints;
     memset(reinterpret_cast<char*>(&hints), 0, sizeof(hints));
     hints.ai_family = static_cast<int>(mSocketInfo.socketFamily);
     hints.ai_socktype = static_cast<int>(mSocketInfo.socketType);
     hints.ai_protocol = static_cast<int>(mSocketInfo.socketProtocol);
     return hints;
}
