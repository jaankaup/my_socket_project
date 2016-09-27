#include <stdexcept>
#include <functional>
#include <chrono>
#include <thread>
#include "socket.h"
#include "error.h"

Socket::Socket(const SocketFamily sf, const SocketType st, const SocketProtocol sp)
{
    mSocketInfo.socketFamily = sf;
    mSocketInfo.socketType = st;
    mSocketInfo.socketProtocol = sp;
    // Ei luoda vielä mSocketia.
}

Socket::~Socket()
{

}

void Socket::Connect(const std::string& address, const std::string& port)
{
    /*
     * Luodaan addrinfoja. Privaatti attribuutti hints_ on vihje siitä millainen yhteys halutaan muodostaa.
     * Result on linkitetty lista addrinfoja, joiden sisältö saadaan myöhemmin getaddrinfo funktiosta serveriltä.
     * Linkitetystä listasta etsitään sellainen addrinfo jolla saadaan muodostettua yhteys.
     * Ptr on pointeri jolla käydään läpi resultia kunnes/jos löytyy sopiva addrinfo jolla yhteydenmuodostus onnistuu.
     */
     std::unique_ptr<addrinfo,std::function<void(addrinfo*)>>
            pResult(nullptr,[](addrinfo* ar) {freeaddrinfo(ar);});
     //std::unique_ptr<addrinfo> ptr;

     addrinfo* result = nullptr;
     addrinfo* ptr = nullptr;

     // Luodaan oma addrinfo, jolla filteroidaan halutunlaisia yhteyksiä.
     addrinfo hints;
     memset(reinterpret_cast<char*>(&hints), 0, sizeof(hints));
     hints.ai_family = static_cast<int>(mSocketInfo.socketFamily);
     hints.ai_socktype = static_cast<int>(mSocketInfo.socketType);
     hints.ai_protocol = static_cast<int>(mSocketInfo.socketProtocol);

    /*
     * Selvitetään toisen osapuolen osoite ym. yhteydenottoon tarvittavaa tietoa.
     * Hints on siis vihje siitä millainen yhteys halutaan. Serveriltä
     * saadaan tietoja yhteyksistä joita se tarjoaa ja jotka vastaavat hints:iä.
     * Nämä tiedot menevät linkitettynä listana resulttiin.
    */
    int status = getaddrinfo(address.c_str(), port.c_str(), &hints, &result);

    /* Jotain meni mönkään. */
    if (status != 0)
    {
        freeaddrinfo(result);
        throw std::runtime_error("Socket.Connect : " + SocketError::getError(WSAGetLastError()));
    }

    bool connected = false;

    /* Etsitään toisen yhteysosapuolelta saatujen addrinfo:jen joukosta sellaista joka vastaisi hints:a. */
    for (ptr = result ; ptr != nullptr ; ptr = ptr->ai_next)
    {
        /* Luodaan uusi socket yhteyttä varten. */
        //sockfd_ = CreateSocket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        //if (sockfd_ == INVALID_SOCKET) return false;

        /* Yritetään ottaa yhteyttä luodulla socketilla. */
        status = connect( mSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));

        /* Saatiin yhteys!. */
        if (status == 0)
        {
            connected = true;
            break;
            //CloseSocket();
        }
    }

    freeaddrinfo(result);

    /* Ei saatu yhteyttä serveriin. */
    if (!connected)
    {
        throw std::runtime_error("Socket.Connect : connection failed.");
    }
}

int Socket::Receive(std::string& s)
{
    int recBytes = 0;
    char receiveBuffer[DEFAULT_BUFLEN];

    /* Result on saatujen tavujen määrä, tai SOCKET_ERROR jos tapahtuu virhe. */
    int result = 0;
    do
    {
        /* Stekataan voidaanko socketista lukea. Odotettaan korkeintaan 200 millisekuntia. */
        bool ready = CheckStatus(200, SocketStatus::READ);
        if (!ready) { return recBytes; }

        result = recv(mSocket, receiveBuffer, sizeof(receiveBuffer), 0);

        if (result > 0) {
            if (result == sizeof(receiveBuffer))
            {
                s.append(receiveBuffer, 0, result);
                recBytes += result;
                continue;
            }
            /* Lisätään '\0' bufferiin oikeaan kohtaan jotta saadaan stringiin oikea data. Muuten saattaa tulla roskaa mukaan stringiin.
             * Koska edellä tarkistettiin tilanne että vastaanotettu data on yhtä suuri kuin puskurin tila, niin tässä on nyt tilanne se, että
             * tavuja on otettu puskuriin vähemmän kuin puskurissa on tilaa. Tässä on nyt päätelty että enempää dataa ei tule.
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
            if (error == WSAEWOULDBLOCK) {
                return recBytes;
            }
            throw std::runtime_error("Socket.Receive: " + SocketError::getError(error));
            //return result;
        };
    } while (result > 0);
    return recBytes;
}

int Socket::ReceiveFrom(std::string& s, EndPoint& remote)
{
    int recBytes = 0;
    char receiveBuffer[DEFAULT_BUFLEN];
    int addr_len = sizeof(struct sockaddr_in);

    /* Result on saatujen tavujen määrä, tai SOCKET_ERROR jos tapahtuu virhe. */
    int result = 0;
    do
    {
        /* Stekataan voidaanko socketista lukea. Odotetaan korkeintaan 200 millisekuntia. */
        bool ready = CheckStatus(200, SocketStatus::READ);
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
            //return result;
        };

    } while (result > 0);
    return recBytes;
}

int Socket::Send(const char* data)
{
    bool ready = CheckStatus(10, SocketStatus::WRITE);
    if (!ready) { std::cout << "EI VOI KIRJOITTAA" << std::endl; return 0; }

    int result = send(sockfd_, data, static_cast<int>(strlen(data)), 0);

    if (result == SOCKET_ERROR)
    {
        std::cout << "Virhe Socket::Send() -> send() epäonnistui." << std::endl;
        Error::PrintError(WSAGetLastError());
        CloseSocket();
        return result;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::SendTo(const std::string& data, EndPoint& remote)
{
    return SendTo(data.c_str(), remote);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Socket::SendTo(const char* data, EndPoint& remote)
{
    bool ready = CheckStatus(10, SocketStatus::WRITE);
    if (!ready) { std::cout << "EI VOI KIRJOITTAA" << std::endl; return 0; }

    int result = sendto(sockfd_,
                        data,
                        static_cast<int>(strlen(data)),
                        0,
                        reinterpret_cast<sockaddr*>(remote()),
                        sizeof(struct sockaddr_in));

    if (result == SOCKET_ERROR)
    {
        std::cout << "Virhe Socket::SendTo() -> send() epäonnistui." << std::endl;
        Error::PrintError(WSAGetLastError());
        CloseSocket();
        return result;
    }

    return result;
}

void Socket::Close()
{
    // Jos socket on invalid-tilassa, niin poistutaan.
    if (sockfd_ == INVALID_SOCKET) return;
    int result = closesocket(mSocket);
    if (result != 0)
    {
        throw std::runtime_error(SocketError::getError(WSAGetLastError()));
    }
    mSocket = INVALID_SOCKET;
}

void Socket::CreateSocket()
{
    SOCKET s = socket(static_cast<int>(mSocketInfo.socketFamily),
                      static_cast<int>(mSocketInfo.socketType),
                      static_cast<int>(mSocketInfo.socketProtocol));

    if (s == INVALID_SOCKET) throw std::runtime_error(SocketError::getError(WSAGetLastError()));
    else mSocket = s;
}

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
        throw std::runtime_error("Socket.SetNonBlockingStatus : " + status);
    }
    return true;
}

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

bool Socket::CheckStatus(const int timeLimit_msec, const SocketStatus status)
{
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
        if (result == 0) return false; // Aikakatkaisu
    }
    while (result == SOCKET_ERROR && errno == EINTR);
    if (result > 0 && FD_ISSET(mSocket, &sockets)) return true;
    return false;
}
