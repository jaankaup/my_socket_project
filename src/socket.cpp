#include "socket.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Socket::Socket(const SocketFamily sf, const SocketType st, const SocketProtocol sp)
{
    // Alustetaan WinSock. WinSock on singleton, joten ylim‰‰r‰iset WinSock alustukset eiv‰t haittaa.
    WinSock::Create();

    // Asetetaan socketin parametrit.
    mSocketInfo.socketFamily = sf;
    mSocketInfo.socketType = st;
    mSocketInfo.socketProtocol = sp;
    mSendBufferLength = DEFAULT_BUFLEN;
    mReceiveBufferLength = DEFAULT_BUFLEN;
    mReceiveTimeout = DEFAULT_RECEIVE_TIMEOUT;

    // Luodaan varsinainen socket.
    CreateSocket();
    // Asetetaan oletusarvoisesti socket blocking tilaan.
    SetNonBlockingStatus(false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Socket::~Socket()
{

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
    /*
     * Luodaan addrinfoja. Privaatti attribuutti hints_ on vihje siit‰ millainen yhteys halutaan muodostaa.
     * Result on linkitetty lista addrinfoja, joiden sis‰ltˆ saadaan myˆhemmin getaddrinfo funktiosta serverilt‰.
     * Linkitetyst‰ listasta etsit‰‰n sellainen addrinfo jolla saadaan muodostettua yhteys.
     * Ptr on pointeri jolla k‰yd‰‰n l‰pi resultia kunnes/jos lˆytyy sopiva addrinfo jolla yhteydenmuodostus onnistuu.
     */
     // TODO: jos j‰‰ aikaan, niin yrit‰ saada toimimaan smartpointer, niin ei tarvitse huolehtia resurssien
     // vapauttamisesta jos tulee poikkeus.
     //std::unique_ptr<addrinfo,std::function<void(addrinfo*)>>
     //       pResult(nullptr,[](addrinfo* ar) {freeaddrinfo(ar);});

     addrinfo* result = nullptr;
     addrinfo* ptr = nullptr;

     // Luodaan oma addrinfo, jolla filteroidaan halutunlaisia yhteyksi‰.
     addrinfo hints = CreateHints();

     // Muutetaan portin numero c-merkkijonoksi.
     std::string s = std::to_string(port);
     char const *portCString = s.c_str();

    /*
     * Selvitet‰‰n toisen osapuolen osoite ym. yhteydenottoon tarvittavaa tietoa.
     * Hints on siis vihje siit‰ millainen yhteys halutaan. Serverilt‰
     * saadaan tietoja yhteyksist‰ joita se tarjoaa ja jotka vastaavat hints:i‰.
     * N‰m‰ tiedot menev‰t linkitettyn‰ listana resulttiin.
    */
    int status = getaddrinfo(address.c_str(), portCString, &hints, &result);

    /* Jotain meni mˆnk‰‰n. */
    if (status != 0)
    {
        // Vapautetaan resurssit, sill‰ seuraavaksi heitet‰‰n poikkeus.
        freeaddrinfo(result);
        throw std::runtime_error("Socket::Connect : " + SocketError::getError(WSAGetLastError()));
    }

    // Flagi, joka kertoo sen onko yhteys muodostettu.
    bool connected = false;

    /* Etsit‰‰n toisen yhteysosapuolelta saatujen addrinfo:jen joukosta sellaista joka vastaisi hints:a.
     * Hyv‰skyt‰‰n heti ensimm‰inen vastaantuleva yhteys.
     */
    for (ptr = result ; ptr != nullptr ; ptr = ptr->ai_next)
    {
        /* Pit‰‰kˆ luoda uusi socket joka kerta? */
        //CreateSocket(); //TODO: k‰sittele mahdollinen runtime_error. Ts. vapauta resurssit.
        //if (mSocket == INVALID_SOCKET) return false;

        /* Yritet‰‰n ottaa yhteytt‰ luodulla socketilla. */
        status = connect( mSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));

        /* Saatiin yhteys!. */
        if (status == 0)
        {
            connected = true;
            break;
        }
    }

    // Vapautetaan resurssit.
    freeaddrinfo(result);

    /* Ei saatu yhteytt‰ serveriin. Heitet‰‰n poikkeus.*/
    if (!connected)
    {
        throw std::runtime_error("Socket::Connect : connection failed.");
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::Receive(std::string& s)
{
    int recBytes = 0;
    char receiveBuffer[mReceiveBufferLength];

    bool infinity = (mReceiveTimeout <= 0);

    /* Result on saatujen tavujen m‰‰r‰, tai SOCKET_ERROR jos tapahtuu virhe. */
    int result = 0;
    do
    {
        /* Tsekataan voidaanko socketista lukea. Jos mReceiveTimeout > 0 niin odotetaan
         * korkeintaan mReceiveTimeout millisekuntia kun selvitet‰‰n voiko socketista lukea. */
        bool ready = true;
        if (!infinity) ready = CheckStatus(mReceiveTimeout, SocketStatus::READ);
        if (!ready) { return recBytes; }

        if (infinity) result = recv(mSocket, receiveBuffer, sizeof(receiveBuffer), MSG_WAITALL);
        else result = recv(mSocket, receiveBuffer, sizeof(receiveBuffer), 0);

        // Jos puskurissa on tavaraa, niin lis‰t‰‰n puskurin sis‰ltˆ s:‰‰n ja
        // lis‰t‰‰n luettujen tavujen lukum‰‰r‰ recBytesiin.
        if (result > 0) {
            if (result == sizeof(receiveBuffer))
            {
                s.append(receiveBuffer, 0, result);
                recBytes += result;
                continue;
            }
            /* Lis‰t‰‰n '\0' bufferiin oikeaan kohtaan jotta saadaan stringiin oikea data. Muuten saattaa tulla roskaa mukaan stringiin.
             * Koska edell‰ tarkistettiin tilanne ett‰ vastaanotettu data on yht‰ suuri kuin puskurin tila, niin t‰ss‰ on nyt tilanne se, ett‰
             * tavuja on otettu puskuriin v‰hemm‰n kuin mit‰ puskurissa on tilaa. T‰ss‰ on nyt p‰‰telty ett‰ enemp‰‰ dataa ei tule.
             */
            receiveBuffer[result] = '\0';
            recBytes += result;
            s += receiveBuffer;
            return recBytes;
        }
        else if (result == 0) { break; }
        else if (result == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            // WSAEWOULDBLOCK ei ole oikeastaan virhe. Jos socket on nonblockin tilassa ja juuri t‰ll‰ hetkell‰
            // puskurissa ei ole mit‰‰n luettavaa, niin palautetaan nykyinen sis‰ltˆ, eik‰ j‰‰d‰ nyt lukemaan lis‰‰.
            // Socketin pit‰isi k‰yd‰ lukemassa purskuria uudestaan hieman myˆhemmin, jotta kaikki mahdollinen
            // data saataisiin luettua.
            if (error == WSAEWOULDBLOCK) {
                return recBytes;
            }
            throw std::runtime_error("Socket.Receive: " + SocketError::getError(error));
            //return result;
        };
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
            throw std::runtime_error("Socket.ReceiveFrom: " + SocketError::getError(error));
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
        throw std::runtime_error("Socket::Send(). " + SocketError::getError(WSAGetLastError()));
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
        throw std::runtime_error("Socket::SendTo(). " + SocketError::getError(WSAGetLastError()));
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
        throw std::runtime_error(SocketError::getError(WSAGetLastError()));
    }
    mSocket = INVALID_SOCKET;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Socket Socket::Accept()
{
    SOCKET result = accept(mSocket, nullptr, nullptr);
    if (result == INVALID_SOCKET)
    {
        throw std::runtime_error("Socket::Accept. " + SocketError::getError(WSAGetLastError()));
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

    if (s == INVALID_SOCKET) throw std::runtime_error(SocketError::getError(WSAGetLastError()));
    else mSocket = s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Socket::SetNonBlockingStatus(bool status)
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
        throw std::runtime_error("Socket::SetNonBlockingStatus : " + status);
    }
    return true;
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
