#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <memory>
#include "socketHeaders.h"
#include "endpoint.h"

class EndPoint;

constexpr int DEFAULT_BUFLEN = 1028;
constexpr const char* DEFAULT_PORT = "25000";

/// Socketti perheeet.
enum class SocketFamily  : int {SOCKET_AF_INET = AF_INET};
/// Socketti tyypit.
enum class SocketType    : int {SOCKET_SOCKET_STREAM = SOCK_STREAM,
                                SOCKET_SOCK_DGRAM = SOCK_DGRAM};
/// Socketti protokollat.
enum class SocketProtocol: int {SOCKET_IPPROTO_TCP = IPPROTO_TCP,
                                SOCKET_IPPROTO_UDF = IPPROTO_UDP};
/// Luettelo sockettiin liittyvist‰ tiloista.
enum class SocketStatus {READ,WRITE /*, ERROR = 2*/};

/*
 * Yksinkertainen socket luokka.
 */
class Socket
{
    public:
        /// Rakentaja. Aiheuttaa runtime_error:in, jos socketin luonti ep‰onnistuu.
        Socket(const SocketFamily sf, const SocketType st, const SocketProtocol sp);
        /// Destruktori.
        virtual ~Socket();

        /// Luo yhteyden is‰nt‰‰n. Aiheuttaa poikkeuksen, jos ep‰onnistuu.
        void Connect(const std::string& address, const std::string& port);

        /// Sulkee socketin ja vapauttaa socketin k‰ytt‰m‰t resurssit. Aiheuttaa runtime_error:in, jos
        /// sulkeminen ep‰onnistuu.
        void Close();

        /// Ottaa vastaan dataa socket:n kautta. @int on vastaanotettujen tavujen m‰‰r‰. @data on
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

        /// Asettaa socketin non-blocking tilaan jos status == true tai blocking tilaan jo status == false.
        bool SetNonBlockingStatus(bool status);

    protected:

    private:
        /// Varsinainen socket.
        SOCKET mSocket = INVALID_SOCKET;

        /// Socketin endpoint.
        EndPoint mEndpoint;

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
