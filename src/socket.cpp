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
    if (mm < 0)
        throw SocketException("Socket::SetReceiveTimeout: Given argument mm must be > 0. Now its " + std::to_string(mm) + ".");
    mReceiveTimeout = mm;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int Socket::GetReceiveBufferSize() const
{
    return mReceiveBufferLength;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::GetSendTimeout() const
{
    return mSendTimeout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::SetSendTimeout(const int mm)
{
    if (mm < 0)
        throw SocketException("Socket::SetSendTimeout: Given argument mm must be > 0. Now its " + std::to_string(mm) + ".");
    mSendTimeout = mm;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Socket::IsBlocking() const
{
    return mIsBlocking;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::SetReceiveBufferSize(const unsigned int recSize)
{
    if (recSize < 0)
        throw SocketException("Socket::SetReceiveBufferSize: Given argument recSize must be > 0. Now its " + std::to_string(recSize) + ".");

    mReceiveBufferLength = recSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::Available()
{
    return GetAvailableBytes(0);
}

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
        throw SocketException("Socket::Connect : " + SocketError::getError(WSAGetLastError()));
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
            mConnected = true;
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
        // If receiveTimeout is zero or an negative number, the socket is going to wait forever to complete its task.
        // Otherwise the socket is going to wait for mReceiveTimeout amount of time to wait and then tell if its ready
        // to read. TODO: the time check is a wrong place.
        if (!infinity) ready = CheckStatus(mReceiveTimeout, SocketStatus::READ);
        if (!ready) { return recBytes; }

        // A different call for forever waiting operation and timed operation.
        // MSG_WAITALL continues only, if (1) recBuffer is full or (2) the host has closed the connection or
        // (3) if the request has been canceled or an error occurred.
        if (infinity)
        {
            result = recv(mSocket, receiveBuffer, sizeof(receiveBuffer), MSG_WAITALL);

            // TODO: if the connection is closed from the remote end point, there will be problems.
            // We must check some how that condition.
        }
        else
        {
            result = recv(mSocket, receiveBuffer, sizeof(receiveBuffer), 0);
            if (result == 0) throw SocketException("Seems that the remote socket disconnected.");
        }

        //std::cout << "result = " << result << std::endl;
        //if (infinity && result == 0) throw SocketException("Toinen osapuoli lahti latkimaan.");

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
            // if (error == WSAECONNRESET) { std::cout << "Connection_reset_by_peer_exception()" << std::endl; }
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

    /* Amount of data received or socket error. */
    int result = 0;

    // Just a typecast so we can call recvfrom.
    sockaddr* pTemp = reinterpret_cast<sockaddr*>(&remote.mEndPointData);

    // This switch case is not finished. It's doesn't do anything right now. This could be a better way to implement recvfrom
    // by first checking the status of socket and then choose the right recvfrom function with right flags.
    switch (mConnected)
    {
        case true: // IsConnected.

            switch(mIsBlocking)
            {
                case true:
                    throw std::runtime_error("Socket::ReceiveFrom: not implemented connected blocking case.");
                    break;

                case false:
                    throw std::runtime_error("Socket::ReceiveFrom: not implemented connected non-blocking case.");
                    break;
            }
        break; // IsConnected ends.

        case false: // Not connected.

            switch(mIsBlocking)
            {
                case true:
                break;

                case false:
                break;
            }
        break; // NonBlocking ends.

    } // switch ends.

    do
    {
        /* Can we read? */
        bool ready = CheckStatus(mReceiveTimeout, SocketStatus::READ);
        int bytesAvailable = GetAvailableBytes(0);
        if (IsBlocking()) ready = true; // blocking socket doesn't return now.
        if (!ready) { return recBytes; } // Nothing to read. Non-blocking returns now;

        // TODOOO!: This can't be done like this. Just read once and if the incoming data doesn't fit the rec-buffer
        // then throw an exception. This works for ITKP104 but not in a general application.
        result = recvfrom(mSocket,
                          receiveBuffer,
                          sizeof(receiveBuffer),
                          0,
                          pTemp,
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
            else throw SocketException("Socket::ReceiveFrom: " + SocketError::getError(error));
        };
    } while (result > 0);
    return recBytes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::Send(const std::string& data)
{
    const char* dataCFormat = data.c_str();
    bool ready = CheckStatus(mSendTimeout, SocketStatus::WRITE);
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
    bool ready = CheckStatus(mSendTimeout, SocketStatus::WRITE);

    // Just a typecast.
    sockaddr* pTemp = reinterpret_cast<sockaddr*>(&remote.mEndPointData);

    if (!ready) throw SocketException("Socket::SendTo: Socket cannot send data.");

    int result = sendto(mSocket,
                        dataCString,
                        static_cast<int>(strlen(dataCString)),
                        0,
                        pTemp,
                        sizeof(struct sockaddr));

    if (result == SOCKET_ERROR)
    {
        SocketException("Socket::SendTo(): " + SocketError::getError(WSAGetLastError()));
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::Close()
{
    mConnected = false;
    if (mSocket == INVALID_SOCKET) return;
    int result = closesocket(mSocket);
    if (result != 0)
    {
        SocketException(SocketError::getError(WSAGetLastError()));
    }
    mSocket = INVALID_SOCKET;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Socket> Socket::Accept()
{
    if (!mIsBound) throw SocketException("Socket::Accept: This socket is not bound.");

    // The information about the connecting entity.
    sockaddr clientInfo;
    auto sizeOfClientInfo = static_cast<int>(sizeof(sockaddr));

    SOCKET result = accept(mSocket, &clientInfo, &sizeOfClientInfo);
    if (result == INVALID_SOCKET)
    {
        SocketException("Socket::Accept. " + SocketError::getError(WSAGetLastError()));
    }

    EndPoint remote;

    // We need to do some convert sockaddr to sockaddr_in to get remote address and port.
    sockaddr_in* conversion = reinterpret_cast<sockaddr_in*>(&clientInfo);
    remote.mEndPointData = *conversion;

    std::unique_ptr<Socket> s(new Socket(mSocketInfo.socketFamily,mSocketInfo.socketType,mSocketInfo.socketProtocol));

    // This might be a bit dangerous. We are modifying the EndPoint objects private attributes from socket code.
    // If the implementation of endpoint is changed, then there might be a chance that this code will break something.
    // Socket if a friend class to endpoint, so this is now possible.
    s->mRemoteEndPoint = remote;
    s->mSocket = result;
    s->mConnected = true;

    return s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::Listen(const int backlog)
{
    if (!mIsBound) throw SocketException("Socket::Listen: This socket is not bound.");
    if (backlog > SOMAXCONN) mBacklog = SOMAXCONN;
    else mBacklog = backlog;

    if (listen(mSocket,mBacklog) == SOCKET_ERROR)
    {
        throw SocketException("Socket::Listen: " + SocketError::getError(WSAGetLastError()));
    }
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
        throw SocketException("Socket::SetNonBlockingStatus : " + status); // TODO: better message.
    }
    mIsBlocking = !status;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::GetAvailableBytes(const unsigned int mm_seconds)
{
    using namespace std::chrono;

    u_long bytesAvailable = 0;
    milliseconds timeToWait(mm_seconds);
    //auto t1 = high_resolution_clock::now();

    int result = ioctlsocket(mSocket, FIONREAD, &bytesAvailable);

    // Successful.
    if (result == 0)
    {
        return static_cast<int>(bytesAvailable);
    }
    else if (result == SOCKET_ERROR)
    {
        throw SocketException(SocketError::getError(WSAGetLastError()));
    }
    throw std::runtime_error("Socket::GetAvailavleBytes: result > 0.");
    /*
    {
        std::this_thread::sleep_for(milliseconds(mm_seconds));
        auto t2 = high_resolution_clock::now();
        milliseconds delta = duration_cast<milliseconds>(t2 - t1);
        if (timeToWait.count()-delta.count() < 0) return bytesAvailable;
    }
    */
    //return static_cast<int>(bytesAvailable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Socket::Bind(const EndPoint& ep)
{
    if (mIsBound) throw SocketException("Socket::Bind: This socket is already bound.");

    addrinfo* result = nullptr;
    addrinfo* ptr = nullptr;

    addrinfo hints = CreateHints();

    // We need to set AI_PASSIVE flag if want to call bind-function.
    hints.ai_flags = AI_PASSIVE;

    /*
     * We need to convert ip4 adresses to a right format so we are able to call geteaddrinfo.
     * Because of AI_PASSIVE, we must do the following: "0.0.0.0" must be NULL and "127.0.0.1" must be "localhost".
     */
    const char* ip = ep.GetIpAddress().c_str();

    if (strcmp(ip, "0.0.0.0") == 0) ip = NULL;
    else if (strcmp(ip, "127.0.0.1") == 0) ip = "localhost";

    int status = getaddrinfo(ip,
                             std::to_string(ep.GetPortNumber()).c_str(),
                             &hints,
                             &result);

    if (status != 0)
    {
        freeaddrinfo(result);
        throw SocketException("Socket::Bind: " + SocketError::getError(WSAGetLastError()));
    }

    bool succeed = false;
    std::vector<std::string> errors;

    for (ptr = result ; ptr != nullptr ; ptr = ptr->ai_next)
    {
        status = bind(mSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
        if (status != SOCKET_ERROR)
        {
            // Success! Now this socket is bound and has a local end point.
            succeed = true;
            mIsBound = true;
            mLocalEndPoint = ep;
            break;
        }
        else errors.push_back(SocketError::getError(WSAGetLastError()));
    }

    freeaddrinfo(result);

    if (!succeed)
    {
        std::string allErrors;
        for (int i=0; i<errors.size(); ++i)
        {
            allErrors += std::to_string(i+1) + ". attempt: " + errors[0];
        }
        throw SocketException("Socket::Bind: An error occurred when binding.\n" + allErrors);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EndPoint Socket::GetLocalEndPoint() const
{
    if (!mIsBound) throw SocketException("Socket::GetLocalEndPoint: This socket is not bound.");
    return mLocalEndPoint;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EndPoint Socket::GetRemoteEndPoint() const
{
    if (!mConnected) throw SocketException("Socket::GetRemoteEndPoint: The socket is not connected.");
    return mRemoteEndPoint;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Socket::CheckStatus(const int timeLimit_msec, const SocketStatus status)
{
    // Tänne ei päädytä, jos mReceiveTimeout < 0.
    assert(mReceiveTimeout>=0);

    struct timeval tv;
    /* Struct joka pitää sisällään socketteja. Tässä tapauksessa vain tämä socketti.*/
    fd_set sockets;

    tv.tv_sec = 0;
    tv.tv_usec = timeLimit_msec*1000;

    int result;

    do {
        /* Tyhjennetään sockets. */
        FD_ZERO(&sockets);
        /* Laitetaan socket sockets:iin. */
        FD_SET(mSocket, &sockets);
        /* Jos halutaan tsekata että onko socketilla jotain luettavaa... */
        if (status == SocketStatus::READ)
        {
            result = select(mSocket+1, &sockets, NULL, NULL, &tv);
        }

        /* Jos halutaan tsekata voiko socket lähettää dataa... */
        else if (status == SocketStatus::WRITE)
        {
            result = select(mSocket+1, NULL, &sockets, NULL, &tv);
        }

        /* Jos halutaan stekata onko socketilla jokin virhe. EI TESTATTU! */
        else if (status == SocketStatus::SOCKERR)
        {
            result = select(mSocket+1, NULL, NULL, &sockets, &tv);
        }

        if (result == 0) return false; // Aikakatkaisu.
    }

    // Lets wait until result is a socket error and a signal is caught.
    while (result == SOCKET_ERROR && errno == EINTR);

    if (result > 0 && FD_ISSET(mSocket, &sockets)) return true;

    // Does this work?
    // if (result == 0 && FD_ISSET(mSocket, &sockets) && status == SocketStatus::READ)
    // {
    //     throw SocketException("Socket::CheckStatus: Peer disconnected.");
    // }
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

