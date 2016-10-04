#include <iostream>
#include "socket.h"
#include "winsock.h"
#include "misc.h"

int main()
{
    //WinSock::Create();

    Socket s(SocketFamily::SOCKET_AF_INET,
             SocketType::SOCKET_STREAM,
             SocketProtocol::SOCKET_TCP);

    std::string sendData = "GET / HTTP/1.1\r\nHost: www.jyu.fi\r\n\r\n";
    s.Connect("www.jyu.fi",80);
    s.Send(sendData);
    std::cout << "ok" << std::endl;
    s.SetReceiveTimeout(200);
    s.SetNonBlockingStatus(true);
    s.SetReceiveBufferSize(1024);

    std::string data;
    int dataInBytes = s.Receive(data);
    std::cout << dataInBytes << std::endl;
    std::cout << data << std::endl;
    s.Close();
    Misc::ReadKey("Paina jotain lopettaaksesi>");
    return 0;
}
