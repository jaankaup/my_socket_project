#include <iostream>
#include "socket.h"
#include "winsock.h"
#include "misc.h"
#include <sstream>


int main()
{
    //WinSock::Create();

    Socket s(SocketFamily::SOCKET_AF_INET,
             SocketType::SOCKET_STREAM,
             SocketProtocol::SOCKET_TCP);
    /*
    std::string sendData = "GET / HTTP/1.1\r\nHost: www.jyu.fi\r\n\r\n";
    s.Connect("www.jyu.fi",80);
    s.Send(sendData);
    std::cout << "ok" << std::endl;
    s.SetReceiveTimeout(20);
    s.SetNonBlockingStatus(true);
    s.SetReceiveBufferSize(1024);

    std::string data;
    int dataInBytes = s.Receive(data);
    std::cout << dataInBytes << std::endl;
    std::cout << data << std::endl;
    s.Close();
    //std::cout << "MSG_DONTWAIT = " << MSG_DONTWAIT << std::endl;
    Misc::ReadKey("Paina jotain lopettaaksesi>");
    */
    int luku;
    std::string pah = "123";
    std::istringstream ss;
    ss.str("222");
    ss.clear();
    ss.str("333");
    ss.clear();
    ss >> luku;
    auto flags = ss.eof();
    auto flags2 = ss.good();
    std::cout << "Input: " << pah << "; luku = " << luku << "; eof = " << flags << "; good = " << flags2 << std::endl;
    std::cout << std::numeric_limits<u_short>::max() << std::endl;
    return 0;
}
