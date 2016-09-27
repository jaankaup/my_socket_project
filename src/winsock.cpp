#include <stdexcept>
#include <string>
#include "winsock.h"

WinSock::WinSock()
{
    WSAData version;
    WORD word = MAKEWORD(2,2);
    int error = WSAStartup(word, &version);

    if (error != 0)
    {
        std::string errorMessage;

        switch (error)
        {
        case WSASYSNOTREADY:
            errorMessage = "The underlying network subsystem is not ready for network communication.";
            break;

        case WSAVERNOTSUPPORTED:
            errorMessage = "The version of Windows Sockets support requested is not provided by this \n"
                           "particular Windows Sockets implementation.";
            break;

        case WSAEINPROGRESS:
            errorMessage = "A blocking Windows Sockets 1.1 operation is in progress.";
            break;

        case WSAEPROCLIM:
            errorMessage = "A limit on the number of tasks supported by the Windows Sockets \n"
                           "implementation has been reached.";
            break;

        case WSAEFAULT:
            errorMessage = "The lpWSAData parameter is not a valid pointer.";
            break;
        }
        throw new std::runtime_error(errorMessage);
    } // if
}

WinSock::~WinSock()
{
    WSACleanup();
}

WinSock& WinSock::Create()
{
    static WinSock ws;
    return ws;
}
