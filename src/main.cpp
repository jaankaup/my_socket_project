#include <iostream>
#include <sstream>
#include <algorithm>
#include <memory>
#include "socket.h"
#include "winsock.h"
#include "misc.h"

//#define TCPSERVER
//#define TCPASIAKAS
//#define HTMLASIAKAS
#define SMTPASIAKAS

#ifdef SMTPASIAKAS

int main()
{
    Socket s(SocketFamily::SOCKET_AF_INET,
             SocketType::SOCKET_STREAM,
             SocketProtocol::SOCKET_TCP);

    std::string address("127.0.0.1");

    try
    {
        s.Connect(address,25000);
        s.SetReceiveTimeout(500);
    }
    catch (SocketException ex)
    {
        std::cout << "Yhteyden muodostus epaonnistui!" << std::endl;
        std::cout << ex.what() << std::endl;
        return 1;
    }

    while (true)
    {
        int state = 0;
        std::string recBuffer;
        s.Receive(recBuffer);
        std::cout << recBuffer << std::endl;
        /*
        std::vector<std::string> tokenizedMessage = Misc::Tokenize(recBuffer," ");
        if (tokenizedMessage.size() == 0)
        {
            std::cout << "Ei voitu paloitella serverin viestia." << std::endl;
            s.Close();
            return -1;
        }
        */
        const static int smtpStates[8] = {220,250,210,215,354,200,221,500};
        const static std::vector<std::string> smtpStr = {"220", "250","250 2.1.0","2.1.5","354","2.0.0","221", "500"};

        bool stateFound = false;
        for (int i=0; i<smtpStr.size(); ++i)
        {
            if (tokenizedMessage[0] == smtpStr[i])
            {
                state = smtpStates[i];
                stateFound = true;
                break;
            }
        }
        if (!stateFound)
        {
            std::cout << "Ei voitu selvittaa smtp tilaa." << std::endl;
            s.Close();
            return -1;
        }

        std::cout << "tokenizedMessage " << tokenizedMessage[0] << std::endl;

        std::string postFix = "\n\r";
        std::string message;

        switch (state)
        {
            case 220:
                message = Misc::ReadLine("Kirjoita alkutervehdys: ");
                s.Send("HELO " + message + postFix);
                break;

            case 250:
                message = Misc::ReadLine("Kirjoita lahettaja: ");
                s.Send("MAIL FROM: " + message + postFix);
                break;

            case 210:
                message = Misc::ReadLine("Kirjoita vastaanottaja: ");
                s.Send("RCPT TP: " + message + postFix);
            break;

            case 215:
                message = Misc::ReadLine("Kirjoita viesti: ");
                s.Send("DATA: " + message + postFix);
            break;

            case 354:
                s.Send("send email");
            break;

            case 200:
                s.Send("QUIT");
            break;

            case 221:
                s.Send("-");
            break;

            default:
                std::cout << "DEFAULT!" << std::endl;
        }
    }



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
    e.SetPortNumber(24999);
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
        return -1;
    }

    // Luodaan turvallinen pointteri asiakas-sockettiin.
    std::unique_ptr<Socket> client;

    try
    {
        client = server.Accept();
    }
    catch (SocketException ex)
    {
        std::cout << "Accept epaonnistui!" << std::endl;
        std::cout << ex.what() << std::endl;
        server.Close();
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
                std::cout << "Viestin vastaanottaminen epaonnistui!" << std::endl;
                std::cout << ex.what() << std::endl;
                client->Close();
                server.Close();
                return -1;
            }
        }
        while (received == 0);

        std::cout << "Saatiin dataa: " + recBuffer << std::endl;

        try
        {
            client->Send("Janne Kauppinen;" + recBuffer);
        }

        catch (SocketException ex)
        {
            std::cout << "Viestin lahettaminen epaonnistui!" << std::endl;
            std::cout << ex.what() << std::endl;
            client->Close();
            server.Close();
            return -1;
        }
    } // while

    server.Close();
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

    std::string address = "127.0.0.1";

    try
    {
        s.Connect(address,25000);
    }
    catch (SocketException ex)
    {
        std::cout << "Yhteyden muodostus epaonnistui!" << std::endl;
        std::cout << ex.what() << std::endl;
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
    }
}
#endif // HTMLASIAKAS
