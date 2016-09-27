#include <iostream>
#include "socket.h"
#include "winsock.h"



int main()
{
    WinSock::Create();

    Socket s(SocketFamily::SOCKET_AF_INET,
             SocketType::SOCKET_STREAM,
             SocketProtocol::SOCKET_TCP);
    /*
    s.Connect("localhost","25005");
    std::string sendData = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    int tavuja = s.Send(sendData);
    std::cout << tavuja << std::endl;
    std::string saatuData;
    int tavujaSaatu = s.Receive(saatuData);
    std::cout << saatuData << std::endl;
    std::cout << tavujaSaatu << std::endl;
    s.Close();
    */
    std::string sendData = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    s.Connect("www.jyu.fi","80");
    s.Send(sendData);
    std::string data;
    int dataInBytes = s.Receive(data);
    std::cout << dataInBytes << std::endl;
    std::cout << data << std::endl;
    s.Close();

    return 0;
}
