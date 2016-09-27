#include "error.h"

namespace SocketError
{

std::string getError(const int error)
{
    switch (error) {
    case WSATRY_AGAIN: return "WSATRY_AGAIN: A temporary failure in name resolution occurred.";
    case WSAEINVAL:	return "WSAEINVAL :An invalid value was provided for the ai_flags member of the pHints parameter." ;
    case WSANO_RECOVERY: return "WSANO_RECOVERY: A nonrecoverable failure in name resolution occurred." ;
    case WSAEAFNOSUPPORT:	return "WSAEAFNOSUPPORT: The ai_family member of the pHints parameter is not supported." ;
    case WSA_NOT_ENOUGH_MEMORY:	return "WSA_NOT_ENOUGH_MEMORY: A memory allocation failure occurred." ;
    case WSAHOST_NOT_FOUND:	return "WSAHOST_NOT_FOUND: The name does not resolve for the supplied parameters or the pNodeName and pServiceName parameters were not provided." ;
    case WSATYPE_NOT_FOUND:	return "WSATYPE_NOT_FOUND: The pServiceName parameter is not supported for the specified ai_socktype member of the pHints parameter." ;
    case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT: The ai_socktype member of the pHints parameter is not supported." ;
    case WSANOTINITIALISED: return "WSANOTINITIALISED: A successful WSAStartup call must occur before using this function." ;
    case WSAENETDOWN: return "WSAENETDOWN: The network subsystem has failed." ;
    case WSAEACCES: return "WSAEACCES: The requested address is a broadcast address, but the appropriate flag was not set. "
                                 "Call setsockopt with the SO_BROADCAST socket option to enable use of the broadcast address." ;
    case WSAEINTR: return "WSAEINTR: A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall." ;
    case WSAEINPROGRESS: return "WSAEINPROGRESS: A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function." ;
    case WSAEFAULT: return "WSAEFAULT: The buf parameter is not completely contained in a valid part of the user address space." ;
    case WSAENETRESET: return "WSAENETRESET: The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress." ;
    case WSAENOBUFS: return "WSAENOBUFS: No buffer space is available." ;
    case WSAENOTCONN: return "WSAENOTCONN: The socket is not connected." ;
    case WSAENOTSOCK: return "WSAENOTSOCK: The descriptor is not a socket." ;
    case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP: MSG_OOB was specified, but the socket is not stream-style such as type "
                                     "SOCK_STREAM, OOB data is not supported in the communication domain "
                                     "associated with this socket, or the socket is unidirectional and supports only receive "
                                     "operations." ;
    case WSAESHUTDOWN: return "WSAESHUTDOWN: The socket has been shut down; it is not possible to send on a socket after "
                                    "shutdown has been invoked with how set to SD_SEND or SD_BOTH." ;
    case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK: The socket is marked as nonblocking and the requested operation would block." ;
    case WSAEMSGSIZE: return "WSAEMSGSIZE: The socket is message oriented, and the message is larger than the maximum supported by the underlying transport." ;
    case WSAEHOSTUNREACH: return "WSAEHOSTUNREACH: The remote host cannot be reached from this host at this time." ;
    // case WSAEINVAL: return "The socket has not been bound with bind, or an unknown flag was specified, or "
    //                              "MSG_OOB was specified for a socket with SO_OOBINLINE enabled." ;
    case WSAECONNABORTED: return "The virtual circuit was terminated due to a time-out or other failure. The application "
                                       "should close the socket as it is no longer usable." ;
    case WSAECONNRESET: return "The virtual circuit was reset by the remote side executing a hard or abortive close. "
                                     "For UDP sockets, the remote host was unable to deliver a previously sent UDP "
                                     "datagram and responded with a 'Port Unreachable' ICMP packet. The application "
                                     "should close the socket as it is no longer usable." ;
    case WSAETIMEDOUT: return "The connection has been dropped, because of a network failure or because the "
                                    "system on the other end went down without notice." ;
    case WSAEMFILE: return "No more socket descriptors are available." ;
    case WSAEINVALIDPROVIDER: return "The service provider returned a version other than 2.2." ;
    case WSAEINVALIDPROCTABLE: return "The service provider returned an invalid or incomplete procedure table to the"
                                            "WSPStartup." ;
    case WSAEPROTONOSUPPORT: return "The specified protocol is not supported." ;
    case WSAEPROTOTYPE: return "The specified protocol is the wrong type for this socket." ;
    case WSAEPROVIDERFAILEDINIT: return "The service provider failed to initialize. This error is returned if a layered service "
                                              "provider (LSP) or namespace provider was improperly installed or the provider"
                                              "fails to operate correctly." ;
    }
    return "Unknow error.";
}
} // namespace
