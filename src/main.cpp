#include <iostream>
#include <sstream>
#include <algorithm>
#include <memory>
#include <list>
#include <algorithm>
#include <thread>
#include <chrono>
#include <exception>
#include <functional>
#include <random>
#include <chrono>
#include "socket.h"
#include "winsock.h"
#include "misc.h"
#include "endpoint.h"

//#define TCPSERVER
//#define TCPASIAKAS
//#define HTMLASIAKAS
//#define SMTPASIAKAS
//#define UDPSERVER
//#define UDPCLIENT
//#define GAMECLIENT
#define GAMESERVER

#ifdef GAMESERVER

int main()
{
    // Serverin tilat.
    enum class ServerStates {CLOSED = 0, WAIT = 1, GAME = 2, WAIT_ACK = 3, END = 4};
    // Serverin tilat merkkijonoina.
    std::string serverStatesStr[5] = {"CLOSE","WAIT","GAME","WAIT_ACK","END"};

    // Luodaan satunnaisluku.
    using namespace std::chrono;
    auto now = high_resolution_clock::now();
    auto nanos = duration_cast<nanoseconds>(now.time_since_epoch()).count();

    std::mt19937 generator(nanos);
    std::uniform_int_distribution<> dis(0, 10);

    int onnenNumero = dis(generator);
    std::cout << "Voittonumero = " << onnenNumero << std::endl;

    // Luodaan serverin socketti.
    Socket server(SocketFamily::SOCKET_AF_INET,
                  SocketType::SOCKET_DGRAM,
                  SocketProtocol::SOCKET_UDP);

    EndPoint ep;
    ep.SetInterface(Interfaces::LOOPBACK);
    ep.SetPortNumber(9999);

    server.SetReceiveTimeout(200);
    server.SetNonBlockingStatus(true);

    server.Bind(ep);

    bool running = true;

    // Edellinen arvaus.
    int lastGuess = -1;

    // Serverin nykytila.
    ServerStates serverState = ServerStates::WAIT;

    // Serverin tila while-luupin alussa. Tarvitaan ainoastaan tulostusta varten,
    // silla jos tila muuttuu kesken loopin, niin tulostetaan uusi tila seuraavan kerran while-
    // silmukan alussa.
    ServerStates serverStateOld = ServerStates::WAIT;

    struct Player
    {
        EndPoint playerEndPoint;
        std::string name;
        bool myTurn = false;
    };

    // Funktio, joka swappaa pelaajien vuoro-muuttujat painvastaiseksi.
    constexpr auto swapTurns = [](std::vector<Player>& players)
    {
        for (auto& p : players)
        {
            p.myTurn = !p.myTurn;
        }
    };

    // Funktio, joka etsii pelaajan EndPointin avulla ja palauttaa true, jos on pelaajan pelivuoro, muutoin palauttaa falsen.
    // Heittaa poikkeuksen, jos pelaajaa ei loydy.
    constexpr auto findPlayer = [](std::vector<Player>& players, const EndPoint& ep)
    {
        for (auto& p : players)
        {
            if (p.playerEndPoint == ep) return p;
        }
        throw std::runtime_error("findPlayer: no such player.");
    };

    // Funktio, joka etsii pelaajan EndPointin avulla ja palauttaa true, jos on pelaajan pelivuoro, muutoin palauttaa falsen.
    // Heittaa poikkeuksen, jos pelaajaa ei loydy.
    constexpr auto hasTurn = [](std::vector<Player>& players, const EndPoint& ep)
    {
        for (auto& p : players)
        {
            if (p.playerEndPoint == ep) return p.myTurn;
        }
        throw std::runtime_error("findPlayer: no such player.");
    };

    // Pelaajat.
    std::vector<Player> clients;

    std::cout << "[WAIT] " << std::endl;

    // Paasilmukka.
    while (running)
    {
        // Tilan tulostus, jos tila on hiljattain muuttunut.
        if (serverState != serverStateOld)
        {
            std::cout << "[" << serverStatesStr[static_cast<int>(serverState)] << "] " << std::endl;
            serverStateOld = serverState;
        }

        std::string message;
        EndPoint remote;

        int received = 0;

        try
        {
            received = server.ReceiveFrom(message,remote);
        }

        catch (SocketException ex)
        {
            std::cout << "Virhe received-funktiosta." << std::endl;
            std::cout << ex.what() << std::endl;
            running = false;
            continue;
        }

        if (received == 0) continue;

        // Tulostetaan saatu viesti.
        std::cout << "[" << serverStatesStr[static_cast<int>(serverState)] << "] : ["
                  << remote.GetIpAddress() << "," << remote.GetPortNumber() << "] " << message << std::endl;

        // Pilkottu viesti asiakkaalta.
        auto tokenizedMsg = Misc::Tokenize(message, " ");

        // Luettelo protokollaviestin eri viestivaihtoehdoista.
        enum class MainMessage {JOIN, DATA, ACK, QUIT};

        // Yksi virheilmoituksista. Tama on nyt tassa silla samaa ilmoitusta saattaa tarvita useita kertoja.
        static const std::string WRONG_SYNTAX_ERROR = "ACK 402 Viesti vaaraa muotoa. "
                "Viestin pitaa olla muotoa: viesti VALILYONTI arvo";

        // Viestista ei voitu erottaa edes kahta palaa.
        if (tokenizedMsg.size() < 2)
        {
            server.SendTo(WRONG_SYNTAX_ERROR,remote);
            continue;
        }

        // Protokollaviestin tyypin maarittely.
        struct ProtocolMessage
        {
            MainMessage first;
            int second = 0;
        };

        // Luodaan "tyhja" protokollaviesti.
        ProtocolMessage protocolMessage;

        // Yritetaan jasentaa viesti-osuus.
        if (tokenizedMsg[0] == "JOIN") protocolMessage.first = MainMessage::JOIN;
        else if (tokenizedMsg[0] == "DATA") protocolMessage.first = MainMessage::DATA;
        else if (tokenizedMsg[0] == "ACK") protocolMessage.first = MainMessage::ACK;
        else if (tokenizedMsg[0] == "QUIT") protocolMessage.first = MainMessage::QUIT;
        else
        {
            server.SendTo("ACK 400 Viestiosaa (" + tokenizedMsg[0] + ") ei voitu jasentaa.",remote);
            continue;
        }

        // Tahan tulee mahdollinen jasennetty int arvo.
        int value;

        // Yritetaan jasentaa arvo-osuus.
        if (!Misc::ParseNumber(tokenizedMsg[1],value))
        {
            // Vain JOIN voi pitaa sisallaan ei-numeerista arvoa.
            if (protocolMessage.first != MainMessage::JOIN)
            {
                server.SendTo("ACK 400 Arvo-osaa (" + tokenizedMsg[1] + ") ei voitu jasentaa.",remote);
                continue;
            }
        }

        protocolMessage.second = value;

        // Loydettiinko asiakas olemassaolevista asiakkaista. newPlayerfound on huono nimi talle muuttujalle, silla
        // nyt muuttuja kertoo sen, loytyiko jo olemassa oleva pelaaja. Jos jaa aikaa, niin korjaan sen.
        bool newPlayerFound = false;

        for (Player& p : clients)
        {
            if (p.playerEndPoint == remote)
            {
                // Loytyi.
                newPlayerFound = true;
                break;
            }
        }

        // Valiaikainen pelaaja-olio.
        Player tempPlayer;

        // Kieltaydytaan ottamasta uusi asiakas peliin, jos on jokin muu tila kuin WAIT!
        // Taman olisi voinnut laittaa myos tila-automaattiin, mutta nain jalkikateen sen sijoittaminen
        // sinne olisi teettanyt jonkin verran tyota, joten ohjelmoijan laiskuuden ja ajansaaston vuoksi
        // tama tarkastus on nyt tass.
        if (!newPlayerFound && serverState != ServerStates::WAIT)
        {
            server.SendTo("ACK 401 Et voi osallistua peliin.", remote);
            continue;
        }

        // Automaatti.
        switch (serverState)
        {

        // CLOSED tila. Tassa ohjelmassa turha, silla serveri ei ole paalla ollessaan closed tilassa.
        // Silloinkin kun serveri olisi paalla, niin se palauttaisi aina saman virheilmoituksen.
        case ServerStates::CLOSED:
            server.SendTo("ACK 408 Peliserveri on CLOSED tilassa.",remote);
            continue;
            break;

        // WAIT tila.
        case ServerStates::WAIT:

            // Virhetilanne: ensimmainen pelaaja yrittaa liittya uudestaan.
            if ((tokenizedMsg[0] == "JOIN") && (clients.size() == 1) && (clients[0].playerEndPoint == remote))
            {
                server.SendTo("ACK 409 Pelaaja on jo liittynyt peliin.",remote);
                continue;
            }

            // Saatiin uusi potentiaalinen asiakas.
            if (!newPlayerFound)
            {
                if (tokenizedMsg[0] == "JOIN")
                {
                    switch (clients.size())
                    {
                    case 0:
                        tempPlayer.playerEndPoint = remote;
                        tempPlayer.name = tokenizedMsg[1];
                        tempPlayer.myTurn = true; // Annetaan ensimmaiselle pelaajalle aloitusvuoro.
                        clients.push_back(tempPlayer);
                        server.SendTo("ACK 201",remote);
                        continue; // while loop.
                    //break;

                    case 1:
                        tempPlayer.playerEndPoint = remote;
                        tempPlayer.name = tokenizedMsg[1];
                        clients.push_back(tempPlayer);
                        tempPlayer.myTurn = false;
                        server.SendTo("ACK 203 " + clients[0].name,remote);
                        server.SendTo("ACK 202 " + tempPlayer.name, clients[0].playerEndPoint);
                        serverState = ServerStates::GAME; // Eiku pelaamaan.
                        continue; // while loop.
                        //break;
                    }
                }

            } // new player.

            // Virhetilanne: JOIN:ssa saa olla vain JOIN viesteja.
            server.SendTo("ACK 400 Vain JOIN viestit kelpaavat nyt.",remote);
            break; // JOIN


        case ServerStates::GAME:

            if (tokenizedMsg[0] == "DATA")
            {
                // Virhetilanne: Pelaaja yrittaa palauttaa dataa vaikka ei ole hanen vuoronsa.
                if (!hasTurn(clients, remote))
                {
                    server.SendTo("ACK 402 Ei ole sinun vuorosi.",remote);
                    continue;
                }
                else
                {
                    // Laitetaan arvaus muistiin.
                    lastGuess = protocolMessage.second;

                    // Tarkistetaan voittoehto.
                    if (protocolMessage.second == onnenNumero)
                    {
                        // Lahetetaan viesti kummallekin osapuolelle. Oikea vastaus loytyi.
                        for (auto& p : clients)
                        {
                            if (p.myTurn) server.SendTo("ACK 501 Sina voitit pelin!",p.playerEndPoint);
                            else server.SendTo("ACK 502 Vastustaja voitti pelin arvolla " +
                                                   std::to_string(lastGuess) + ".", p.playerEndPoint);
                        }
                        // Palvelin siirtyy end tilaan.
                        serverState = ServerStates::END;
                        continue;
                    }

                    // Voittoa ei tullut.
                    for (auto& p : clients)
                    {
                        // Lahetetaan kummallekin osapuolelle viestit.
                        if (p.myTurn) server.SendTo("ACK 300 DATA OK", p.playerEndPoint);
                        else server.SendTo("DATA " + std::to_string(protocolMessage.second), p.playerEndPoint);
                    }
                    // Palvelin siirtyy WAIT_ACK tilaan odottamaan toisen pelaajan ACK 300 vahvistusta.
                    serverState = ServerStates::WAIT_ACK;
                    continue;
                }
            }

            // VirheTilanne.
            server.SendTo("ACK 400 Vaaranlainen viesti tai jotain.",remote);
            break;

        case ServerStates::WAIT_ACK:
        {
            // Etsitaan pelaaja yhteydenotosta saadun remote endpointin avulla.
            Player currentPlayer = findPlayer(clients,remote);

            // Nyt halutaan ACK 300 kuittaus toiselta osapuolelta jotta voidaan vaihtaa vuoroja ja jatkaa pelia.
            if (protocolMessage.first == MainMessage::ACK &&
                protocolMessage.second == 300 &&
                !currentPlayer.myTurn)

            {
                // Vaihdetaan pelaajien vuorot paikseen.
                swapTurns(clients);
                // Serveri palaa pelitilaan.
                serverState = ServerStates::GAME;
                continue;
            }
            else if (currentPlayer.myTurn)
            {
                server.SendTo("ACK 400 Et saa lahettaa nyt mitaan viesteja.", currentPlayer.playerEndPoint);
                continue;
            }
            // Virhetilanne: Tuli jokin muu viesti kuin ACK 300.
            else server.SendTo("ACK 400 Vaara ACK viesti. Pitaa olla ACK 300.",remote);
            continue;
            break;
        }

        case ServerStates::END:

            // Pudotetaan pelaaja pois kun saadaan kuittaus ACK 500.
            if (protocolMessage.second == 500)
            {
                for (unsigned int i=0; i<clients.size(); ++i)
                {
                    if (clients[i].playerEndPoint == remote)
                    {
                        clients.erase(begin(clients)+i);
                        break;
                    }
                }
            }

            // Ei saatu viestia ACK 500.
            else if (protocolMessage.second != 500)
            {
                server.SendTo("ACK 400 Vaara kuittaus viesti. Vihje: kokeile ACK 500.",remote);
                continue;
            }
            // Ei ole enaa asiakkaita. Lopetetaan.
            if (clients.size() == 0) running = false;

            break;
        }

    } // while-loop.

    Misc::ReadKey("Paina jotain lopettaaksesi.");
    server.Close();

    return 0;
}

#endif // GAMESERVER



#ifdef GAMECLIENT

int main()
{
    enum class ClientStates {CLOSED, JOIN, GAME};

    Socket s(SocketFamily::SOCKET_AF_INET,
             SocketType::SOCKET_DGRAM,
             SocketProtocol::SOCKET_UDP);

    EndPoint serverEndPoint;
    serverEndPoint.SetInterface(Interfaces::LOOPBACK);
    serverEndPoint.SetPortNumber(9999);

    s.SetNonBlockingStatus(true);
    s.SetReceiveTimeout(200);

    bool running = true;
    bool writing = false;

    // Lahdetaan liikenteeseen Closed tilasta.
    ClientStates cState = ClientStates::CLOSED;

    // Naytetaan ilmoitus.
    bool displayMessage = true;

    // Vastustajan nimi.
    std::string opponent;

    while (running)
    {

        if (cState == ClientStates::CLOSED && displayMessage && !writing)
        {
            std::cout << "Kirjoita nimesi:> ";
            std::string prefix = "JOIN ";
            std::thread t(Misc::TypeText2, std::ref(s), std::ref(serverEndPoint), std::ref(writing), prefix);
            t.join();
            displayMessage = false;
        }

        std::string line;
        int received = s.ReceiveFrom(line, serverEndPoint);
        if (received == 0 && cState != ClientStates::CLOSED) continue;
        if (received > 0)
        {
            // POISTA kommentoitu rivi, niin saadaan rerverin viestien tulostus paalle.
            //std::cout << line << std::endl;
            displayMessage = true;
        }

        std::vector<std::string> tokenizedMessage = Misc::Tokenize(line," ");

        enum class PrimaryMessages {ACK,DATA,QUIT};

        static const int secondaryMSG[13] = {201,202,203,300,400,401,402,403,404,407,500,501,502};

        struct ServerMessage
        {
            PrimaryMessages first;
            int second = 0;
        } serverMsg;

        if (tokenizedMessage.size() < 2)
        {
            std::cout << "VIRHE serverin viestin jasentamisessa." << std::endl;
            break;
        }

        // Yritetaan jasentaa serverin viestin ensimmainen pala.
        if (tokenizedMessage[0] == "ACK") serverMsg.first = PrimaryMessages::ACK;
        else if (tokenizedMessage[0] == "DATA") serverMsg.first = PrimaryMessages::DATA;
        else if (tokenizedMessage[0] == "QUIT") serverMsg.first = PrimaryMessages::QUIT;
        else
        {
            std::cout << "EI voitu jasentaa serverin viestin ensimmaista palaa!" << std::endl;
            break;
        }

        // Yritetaan jasentaa serverin viestin toinen pala.
        bool found = false;

        int parsedNumber = -33;

        try
        {
            parsedNumber = std::stoi(tokenizedMessage[1],nullptr,10);
        }
        catch (std::invalid_argument ex)
        {
            std::cout << "Serverin viestin toinen palanen ei ole numero!" << std::endl;
            break;
        }

        // Kaydaan lapi protokollan numeroviestit (viestin toinen pala).
        for (unsigned int i=0; i<sizeof(secondaryMSG)/(sizeof *secondaryMSG); ++i)
        {
            if (secondaryMSG[i] == parsedNumber)
            {
                serverMsg.second = secondaryMSG[i];
                found = true;
                break;
            }
        }

        // Hyvaksytaan DATA:n toiseksi palaseksi muukin numero kuin edellamainitut luvut.
        if (tokenizedMessage[0] == "DATA") found = true;

        if (!found)
        {
            std::cout << "EI voitu jasentaa serverin viestin toiseta palaa!" << std::endl;
            break;
        }

        // Automaatti.
        switch (serverMsg.first)
        {
        case PrimaryMessages::ACK :
            switch (serverMsg.second)
            {
            case 201:
                std::cout << "Odotetaan toista pelaajaa..." << std::endl;
                displayMessage = false;
                cState = ClientStates::JOIN;
                break;

            case 202:
                cState = ClientStates::GAME;
                opponent = tokenizedMessage[2];
                opponent.erase(std::remove_if(begin(opponent),end(opponent),isspace), end(opponent)); // Tyhjat merkit pois.
                std::cout << opponent << " liittyi peliin!" << std::endl;
                displayMessage = true;
                break;

            case 203:
                cState = ClientStates::GAME;
                opponent = tokenizedMessage[2];
                opponent.erase(std::remove_if(begin(opponent),end(opponent),isspace), end(opponent)); // Tyhjat merkit pois.
                std::cout << opponent << " liittyi peliin!" << std::endl;
                std::cout << opponent << " aloittaa pelin!" << std::endl;
                displayMessage = false;
                break;
            case 300:
                std::cout << "Hyva arvaus, mutta vaarin meni. " << opponent << " kokeilee seuraavaksi." << std::endl;
                displayMessage = false;
                break;
            case 401:
                std::cout << "Et voi liittya peliin!" << std::endl;
                displayMessage = true;
                break;
            case 407:
                std::cout << "Antamasi syote ei ollut numero!" << std::endl;
                displayMessage = true;
                break;
            }
            break;

        case PrimaryMessages::DATA :

            std::cout << opponent << " arvasi lukua " << parsedNumber << ", mutta vaarin meni." << std::endl;
            s.SendTo("ACK 300", serverEndPoint);
            break;

        case PrimaryMessages::QUIT :
            switch (serverMsg.second)
            {
            case 501:
                std::cout << "Sina VOITIT pelin talla kertaa!" << std::endl;
                try
                {
                    s.SendTo("ACK 500", serverEndPoint);
                }
                catch (SocketException ex)
                {
                    std::cout << ex.what() << std::endl;
                }
                cState = ClientStates::CLOSED;
                running = false;
                break;

            case 502:
                for (int i=2; i<tokenizedMessage.size (); ++i)
                {
                    std::cout << tokenizedMessage[i] << " ";
                }
                std::cout << std::endl;
                s.SendTo("ACK 500", serverEndPoint);
                cState = ClientStates::CLOSED;
                running = false;
                break;
            }
            break;
        }

        if (cState == ClientStates::JOIN)
        {

        }

        else if (cState == ClientStates::GAME && displayMessage && !writing)
        {
            std::cout << "Arvaa numero:> ";
            std::string prefix = "DATA ";
            std::thread t(Misc::TypeText2, std::ref(s), std::ref(serverEndPoint), std::ref(writing), prefix);
            t.join();
            displayMessage = false;
        }
    }

    //std::thread loppu(Misc::ReadKey, "Paina jotain lopettaaksesi.");
    //loppu.join();

    // Konsolien jakamien resurssien takia syntyy ongelmia, jonka takia on nyt kaksi readkeyta.
    Misc::ReadKey("Paina jotain lopettaaksesi.");
    Misc::ReadKey("Paina jotain lopettaaksesi2.");

    s.Close();
    return 0;
}
#endif // GAMECLIENT

#ifdef UDPCLIENT

int main()
{
    Socket s(SocketFamily::SOCKET_AF_INET,
             SocketType::SOCKET_DGRAM,
             SocketProtocol::SOCKET_UDP);

    EndPoint serverEndPoint;
    serverEndPoint.SetInterface(Interfaces::LOOPBACK);
    serverEndPoint.SetPortNumber(9999);

    s.SetNonBlockingStatus(true);
    s.SetReceiveTimeout(200);

    bool running = true;
    bool writing = false;
    bool quit    = false;

    while (running)
    {
        std::string line;

        // Jos ei kirjoiteta ja nappia painetaan konsolissa, niin polkaistaan TypeText funktio säikeenä käyntiin.
        // Tassa tulee todennakoisesti jotain hassakkaa consolien kanssa, minka takia asiakkaan poistuminen ei
        // aina onnistu.
        if (!writing && Misc::KeyPressed())
        {
            writing = true;
            std::thread t(Misc::TypeText, std::ref(s), std::ref(serverEndPoint), std::ref(writing), std::ref(quit));
            t.detach();
        }

        if (quit) break;

        int received = s.ReceiveFrom(line, serverEndPoint);
        if (received == 0) continue;

        // Tulostetaan serverin viesti.
        if (received > 0) std::cout << line << std::endl;
    }

    s.Close();

    Misc::ReadKey("Paina jotain lopettaaksesi.");

    return 0;
}

#endif // UDPCLIENT

#ifdef UDPSERVER

int main()
{
    Socket server(SocketFamily::SOCKET_AF_INET,
                  SocketType::SOCKET_DGRAM,
                  SocketProtocol::SOCKET_UDP);

    EndPoint ep;
    ep.SetInterface(Interfaces::LOOPBACK);
    ep.SetPortNumber(9999);

    server.SetReceiveTimeout(200);
    server.SetNonBlockingStatus(true);

    server.Bind(ep);

    bool running = true;

    std::list<EndPoint> clients;

    while (running)
    {
        std::string message;
        EndPoint remote;

        int received = 0;

        try
        {
            received = server.ReceiveFrom(message,remote);
        }

        catch (SocketException ex)
        {
            // Mita ilmeisemmin Connection reset by peer virhe (ICMP). Poistetaan kyseinen endpoint.
            // Tama toimisi varmaankin paremmin, jos asiakkaat olisivat fyysisesti eri koneessa.
            // Misc::TypeText saie todennakoisesti aiheuttaa jotain hankaluutta eri bufferien kanssa.
            for (auto elem : clients)
            {
                if (remote == elem) clients.remove(elem);
            }

            std::cout << "Asiakas poistui [" << remote.GetIpAddress() << ":"
                      << remote.GetPortNumber() << "]" << std::endl;
            continue;
        }

        if (received == 0) continue;

        // Loydettiinko asiakas olemassaolevista asiakkaista?
        bool found = std::find(begin(clients),end(clients), remote) != end(clients);

        // Ei loytynyt. Lisataan uusi asiakas listaan.
        if (!found)
        {
            clients.push_back(remote);
            std::cout << "Uusi asiakas [" << remote.GetIpAddress() << ":"
                      << remote.GetPortNumber() << "]" << std::endl;
        }

        std::vector<std::string> splittedMessage = Misc::Tokenize(message,";");

        // Asiakas lahetti vaaranmuotoista dataa.
        if (splittedMessage.size() < 2)
        {
            std::string errorMessage = "Viestin taytyy olla muotoa 'nimi;viesti'.";
            std::cout << errorMessage << std::endl;
            server.SendTo(errorMessage,remote);
            continue;
        }

        std::string parsedName    = splittedMessage[0];
        std::string parsedMessage = splittedMessage[1];

        // Lahetetaan data kaikille asiakkaille.
        for (auto& c : clients)
        {
            server.SendTo(parsedName + ": " + parsedMessage, c);
        }
    }
    Misc::ReadKey("Paina jotain lopettaaksesi.");
}

#endif // UDPSERVER



#ifdef SMTPASIAKAS

int main()
{
    Socket s(SocketFamily::SOCKET_AF_INET,
             SocketType::SOCKET_STREAM,
             SocketProtocol::SOCKET_TCP);

    std::string address("127.0.0.1");

    //s.SetReceiveTimeout(1000);
    //s.SetNonBlockingStatus(true);

    try
    {
        s.Connect(address,25000);
        s.SetReceiveTimeout(1000);
    }
    catch (SocketException ex)
    {
        std::cout << "Yhteyden muodostus epaonnistui!" << std::endl;
        std::cout << ex.what() << std::endl;
        return 1;
    }

    bool running = true;

    while (running)
    {
        std::string recBuffer;
        s.Receive(recBuffer);
        std::cout << recBuffer << std::endl;

        std::vector<std::string> tokenizedMessage = Misc::Tokenize(recBuffer," ");
        if (tokenizedMessage.size() == 0)
        {
            std::cout << "Ei voitu paloitella serverin viestia." << std::endl;
            s.Close();
            return -1;
        }

        const static int smtpStates[8] = {220,250,210,215,354,200,221,500};
        const static std::vector<std::string> smtpStr = {"220","250", "2.1.0","2.1.5","354","2.0.0","221", "500"};

        struct State
        {
            int first = 0;
            int second = 0;
        } smtpState;

        bool stateFound = false;
        for (int i=0; i<smtpStr.size(); ++i)
        {
            // Yritetaan jasentaa ensimmaista palaa.
            if (tokenizedMessage[0] == smtpStr[i])
            {
                smtpState.first = smtpStates[i];
            }

            // Yritetaan jasentaa toista palaa.
            if (tokenizedMessage[1] == smtpStr[i])
            {
                smtpState.second = smtpStates[i];
            }
        }

        const static std::string postFix = "\r\n";
        std::string message;

        switch (smtpState.first)
        {
        case 220:
            message = Misc::ReadLine("Kirjoita alkutervehdys: ");
            s.Send("HELO " + message + postFix);
            break;

        case 250:
            switch (smtpState.second)
            {
            case 200:
                s.Send("QUIT" + postFix);
                //running = false;
                break;
            case 210:
                message = Misc::ReadLine("Kirjoita vastaanottaja: ");
                s.Send("RCPT TO: " + message + postFix);
                break;
            case 215:
                s.Send("DATA" + postFix);
                break;
            default:
                message = Misc::ReadLine("Kirjoita lahettaja: ");
                s.Send("MAIL FROM: " + message + postFix);
            }
            break;

        case 354:
        {
            // case label. On oltava oma blocki kun maaritellaan data ja line, muuten kaantaja suuttuu.
            std::string data;
            while (true)
            {
                std::string line = Misc::ReadLine("Kirjoita viesti (q = lopetus): ");
                if (line == "q") break;
                data += line + postFix;
            }
            s.Send(data + postFix + "." + postFix);
        } // case label end.
        break;

        case 221:
            switch (smtpState.second)
            {
            // Hieman turha tassa? 221 lopeettaa kuitenkin.
            case 200:
                running = false;
                break;
            default:
                running = false;
                break;
            }
            break;

        default:
            std::cout << "Palvelin ja asiakas eivat ymmartaneet toisiaan!" << std::endl;
            s.Send("QUIT" + postFix);

        } // switch
    } // while

    s.Close();
    Misc::ReadLine("Kirjoita jotain ja paina enter lopettaaksesi.");
    return 0;
}

#endif // SMTPASIAKAS

#ifdef TCPSERVER

/* TCP-SERVERI */

int main()
{
    Socket server(SocketFamily::SOCKET_AF_INET,
                  SocketType::SOCKET_STREAM,
                  SocketProtocol::SOCKET_TCP);

    EndPoint e;
    e.SetPortNumber(25000);
    e.SetInterface(Interfaces::LOOPBACK);

    //Socket client;

    try
    {
        server.Bind(e);
    }
    catch (SocketException ex)
    {
        std::cout << "Socket bindaus epaonnistui!" << std::endl;
        std::cout << ex.what() << std::endl;
        server.Close();
        Misc::ReadKey("Paina jotain nappia lopettaaksesi.");
        return 1;
    }

    try
    {
        server.Listen(1);
        std::cout << "Kuunnellaan..." << std::endl;
    }
    catch (SocketException ex)
    {
        std::cout << "Kuuntelu meni pieleen." << std::endl;
        std::cout << ex.what() << std::endl;
        server.Close();
        Misc::ReadKey("Paina jotain nappia lopettaaksesi.");
        return -1;
    }

    // Luodaan turvallinen pointteri asiakas-sockettiin.
    std::unique_ptr<Socket> client;

    try
    {
        client = server.Accept();
        //client->SetReceiveTimeout(0);
    }
    catch (SocketException ex)
    {
        std::cout << "Accept epaonnistui!" << std::endl;
        std::cout << ex.what() << std::endl;
        server.Close();
        Misc::ReadKey("Paina jotain nappia lopettaaksesi.");
        return -1;
    }

    EndPoint remote = client->GetRemoteEndPoint();
    std::cout << "Yhteys luotu osoitteesta: " << remote.GetIpAddress()
              << " ja portista " << remote.GetPortNumber() << "." << std::endl;

    client->SetReceiveTimeout(200);

    // Yhteys muodostettu. Jaadaan juttelemaan.
    while (true)
    {
        std::string recBuffer;
        int received = 0;

        do
        {
            try
            {
                received = client->Receive(recBuffer);
            }
            catch (SocketException ex)
            {
                std::cout << "Yhteys katkaistu osoitteesta: " << remote.GetIpAddress()
                          << " ja portista " << remote.GetPortNumber() << "." << std::endl;
                client->Close();
                goto Loppu;
            }
        }
        while (received == 0);

        std::cout << "Saatiin viesti: " << recBuffer << std::endl;

        try
        {
            client->Send("Janne Kauppinen;" + recBuffer);
        }

        catch (SocketException ex)
        {
            std::cout << "Viestin lahettaminen epaonnistui!" << std::endl;
            std::cout << ex.what() << std::endl;
            client->Close();
            break;
        }
    } // while

Loppu:
    server.Close();
    Misc::ReadKey("Paina jotain nappia lopettaaksesi.");
    return 0;
}

#endif // TCPSERVER

/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef TCPASIAKAS

/* TCP-ASIAKAS */

int main()
{
    Socket s(SocketFamily::SOCKET_AF_INET,
             SocketType::SOCKET_STREAM,
             SocketProtocol::SOCKET_TCP);

    s.SetReceiveTimeout(200);

    std::string address = "127.0.0.1";

    try
    {
        s.Connect(address,25000);
    }
    catch (SocketException ex)
    {
        std::cout << "Yhteyden muodostus epaonnistui!" << std::endl;
        std::cout << ex.what() << std::endl;
        Misc::ReadKey("Paina jotain nappia lopettaaksesi.");
        return 1;
    }

    while (true)
    {
        std::string message = Misc::ReadLine("Kirjoita viesti (q = poistu): ");
        if (message == "q") break;

        try
        {
            s.Send(message);
        }
        catch (SocketException ex)
        {
            std::cout << "Viestin lahetys epaonnistui!" << std::endl;
            std::cout << ex.what() << std::endl;
            break;
        }

        std::string recBuffer;
        try
        {
            s.Receive(recBuffer);
        }
        catch (SocketException ex)
        {
            std::cout << "Ongelmia Receive-funktion kohdalla!" << std::endl;
            std::cout << ex.what() << std::endl;
            break;
        }

        std::vector<std::string> tokenizedString = Misc::Tokenize(recBuffer,";");

        if (tokenizedString.size() < 2)
        {
            std::cout << "Serverilta saatua viestia ei voitu pilkkoa!" << std::endl;
            std::cout << "Serverilta saatu viesti (" << recBuffer << ")." << std::endl;
            break;
        }

        std::cout << "Serverilta saatu viesti 2 osassa: (nimi = " << tokenizedString[0]
                  << ", viesti = " << tokenizedString[1] << ")." << std::endl;
    }

    s.Close();
    Misc::ReadKey("Paina jotain nappia lopettaaksesi.");
    return 0;
}
#endif // TCPASIAKAS

/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef HTMLASIAKAS

/* HTML-ASIAKAS */

int main()
{

    while (true)
    {
        // Luodaan socket.
        Socket s(SocketFamily::SOCKET_AF_INET,
                 SocketType::SOCKET_STREAM,
                 SocketProtocol::SOCKET_TCP);

        // Asestetaan katkaisuajaksi 1 sekunti.
        s.SetReceiveTimeout(1000);

        std::string command = Misc::ReadLine("Anna www-osoite (q = lopetus): ");
        if (command == "q") break;

        std::string sendData = "GET / HTTP/1.1\r\nHost: " + command + "\r\n\r\n";
        std::string buffer;

        try
        {
            s.Connect(command,80);
            s.Send(sendData);
            s.Receive(buffer);
        }

        catch (SocketException ex)
        {
            std::cout << "Antamaasi osoitteeseen ei saatu yhteytta!" << std::endl;
            s.Close();
            continue;
        }

        std::stringstream ss(buffer);
        std::string line;
        bool print = false;

        // Etsitaan ensimmainen sellainen rivi joka on joko tyhja tai sisaltaa ainoastaan white-space merkkeja.
        // Tulostetaan siita rivista lahtien kaikki rivit.
        while (getline(ss, line))
        {
            if (!print)
            {
                bool whiteSpacesOnly = std::all_of(begin(line),end(line),isspace);
                bool lineIsEmpty = line.size() == 0;
                if (whiteSpacesOnly || lineIsEmpty) print = true;
            }
            if (print) std::cout << line << std::endl;
        }
        s.Close();
    } // while

    Misc::ReadKey("Paina jotain nappia lopettaaksesi.");
}
#endif // HTMLASIAKAS
