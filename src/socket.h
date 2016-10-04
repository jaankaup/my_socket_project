#ifndef SOCKET_H
#define SOCKET_H

#include <stdexcept>
#include <functional>
#include <chrono>
#include <thread>
#include <string>
#include <memory>
#include <assert.h>
#include "socketHeaders.h"
#include "socket.h"
#include "error.h"
#include "winsock.h"
#include "endpoint.h"
#include "socketexception.h"

// Otetaan pois, kun ei en‰‰ debugata.
#define NDEBUG

class EndPoint;
class SocketExepction;

/// Oletus porttikoko ja porttinumero.
constexpr unsigned int DEFAULT_BUFLEN = 1028;
constexpr int DEFAULT_PORT = 25000;
constexpr int DEFAULT_RECEIVE_TIMEOUT = 200;

/// Socketti perheeet.
enum class SocketFamily : int {SOCKET_AF_INET = AF_INET};

/// Socketti tyypit.
enum class SocketType : int {SOCKET_STREAM = SOCK_STREAM,
                             SOCKET_DGRAM = SOCK_DGRAM};

/// Socketti protokollat.
enum class SocketProtocol: int {SOCKET_TCP = IPPROTO_TCP,
                                SOCKET_UDF = IPPROTO_UDP};

/// Luettelo sockettiin liittyvist‰ tiloista.
enum class SocketStatus {READ,WRITE /*, ERROR = 2*/};

/*
 * A simple socket class.
 * Author Janne Kauppinen 2016.
 * None right reserved.
 * The implementation of this class is incomplete. This project is just an exercise. Do not use in real programs.
 */
class Socket
{
    public:
        /// Rakentaja. Aiheuttaa runtime_error:in, jos socketin luonti ep‰onnistuu.
        Socket(const SocketFamily sf, const SocketType st, const SocketProtocol sp);
        /// Destruktori.
        virtual ~Socket();

        /// Pakollinen?
        Socket(const SOCKET socket): mSocket(socket) { }

        int GetReceiveTimeout() const;

        void SetReceiveTimeout(const int mm);

        /// Gets a value that indicates whether the Socket is in blocking mode.
        bool IsBlocking() const;

        unsigned int GetReceiveBufferSize() const;
        void SetReceiveBufferSize(const unsigned int recSize);

        /// Move-assigment operaattori. Ts. vanha Socket-olio korvataan other Socket-oliolla.
        /// Onko pakollinen?
        //Socket& operator=(Socket&& other);

        /// Luo yhteyden is‰nt‰‰n. Aiheuttaa runtime_error:in, jos ep‰onnistuu.
        void Connect(const std::string& address, const int port);

        /// Sulkee socketin ja vapauttaa socketin k‰ytt‰m‰t resurssit. Aiheuttaa runtime_error:in, jos
        /// sulkeminen ep‰onnistuu.
        void Close();

        /// Ottaa vastaan dataa socket:n kautta. Functio palauttaa vastaanotettujen tavujen m‰‰r‰n. @data on
        /// merkkijono johon asetetaan saapuva data.
        int Receive(std::string& data);

        int ReceiveFrom(std::string& s, EndPoint& remote);

        int Send(const std::string& data);

        int SendTo(const std::string& data, EndPoint& remote);

        /// Bindataan socket ennalta m‰‰r‰ttyyn porttiin. T‰m‰n j‰lkeen voi alkaa kuunnella ko. porttia
        /// Listen() j‰senfunktiolla.
        /// Palauttaa true jos onnistuu ja false jos ep‰onnistuu. Ep‰onnistuessaan Bind()
        /// sulkee socketin automaattisesti.
        bool Bind();

        /// Hyv‰ksyy yhteyden. Jos kaikki sujuu hyvin, funktio palauttaa uuden socketin
        /// jonka kanssa voidaan "keskustella". Jos tapahtuuu virhe, niin funktio heitt‰‰ runtime_errorin.
        Socket Accept();

        /// Asettaa socketin non-blocking tilaan jos status == true tai blocking tilaan jo status == false.
        void SetNonBlockingStatus(bool status);

    protected:

    private:
        /// Varsinainen socket.
        SOCKET mSocket = INVALID_SOCKET;

        /// Socketin endpoint.
        EndPoint mEndpoint;

        /// Socketin portinnumero.
        int mPortNumber;

        /// Socketin l‰hetyspuskurin koko.
        int mSendBufferLength;

        /// Socketin vastaanottopuskurin koko.
        int mReceiveBufferLength;

        /// Aika, joka odotetaan ennen kuin aikakatkaistaan Receive ja ReceiveFrom j‰senfunktiot.
        int mReceiveTimeout;

        bool mIsBlocking;

        /// Rakenne, jossa s‰ilytet‰‰n socketin tyyppitiedot.
        struct SocketInfo
        {
            SocketFamily socketFamily;
            SocketType socketType;
            SocketProtocol socketProtocol;
        } mSocketInfo;

        /// Luo addrinfon ainoastaan mSocketInfosta. Tarvitaan silloin, kun filteroidaan t‰m‰n socketin
        /// kaltaisia yhteyksi‰. Vrt. Bind() ja Connect().
        addrinfo CreateHints() const;

        /// Luo uuden mSocketin.
        void CreateSocket();

        /// Palauttaa socketin input-queue:ssa olevien tavujen lukum‰‰r‰n eli tavujen m‰‰r‰n mit‰ sill‰ hetkell‰
        /// voidaan lukea. @mm_seconds on maksimi aika millisekunneissa jonka t‰m‰ j‰senfunktio saa aikaa suoritukseen.
        /// N‰in v‰ltet‰‰n loputon odottelu jos tavuja ei tulekaan luettavaksi ollenkaan.
        int GetAvailableBytes(const unsigned int mm_seconds);

        /// Tarkistaa socketin tilan. Jos status = SocketStatus::READ niin funktio palauttaa true jos socket on valmis ottamaan dataa vastaan. Muutoni palauttaa false.
        /// Jos taas satus = SocketStatus::WRITE niin funktio palauttaa true jos socket on valmis l‰hett‰m‰‰n dataa. Muutoin palauttaa false.
        /// @timeLimitMilliSec_msec on aika millisekunteina, joka function odottaa saadakseen vastauksen.
        bool CheckStatus(const int timeLimitMilliSec, const SocketStatus status);

};

#endif // SOCKET_H
