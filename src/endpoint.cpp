#include "endPoint.h"
#include "error.h"

EndPoint::EndPoint()
{
    ResetEndPoint();
}

EndPoint::~EndPoint()
{
    //dtor
}

EndPoint& EndPoint::operator=(const EndPoint& other)
{
    if (this != &other)
    {
        epData_ = other.epData_;
    }
    return *this;
}

bool EndPoint::operator==(const EndPoint& other)
{
    if ( (epData_.sin_family == other.epData_.sin_family) &&
         (epData_.sin_port == other.epData_.sin_port) &&
         (epData_.sin_addr.s_addr == other.epData_.sin_addr.s_addr) )
    {
        return true;
    }

    return false;
}

sockaddr_in* EndPoint::operator()()
{
    return &epData_;
}

void EndPoint::SetIpAddress(int family, const std::string& ipAddress)
{
    //char buffer[INET6_ADDRSTRLEN];
    //int pah = inet_pton();
    //int result = InetPton(family, ipAddress.c_str(), &(epData_.sin_addr));
    int result = InetPton(family, ipAddress.c_str(), &(epData_.sin_addr));
    if (result == 0) throw std::runtime_error("EndPoint.SetIpAddress: InetPton(): problem with ipAddress.");
    else if (result == -1)
    {
        throw std::runtime_error("EndPoint.SetAddress: InetPton() failed.");
    }
}

void EndPoint::SetIpAddress(int family, const IPAddress ip)
{
    epData_.sin_addr.s_addr = htonl(IPs[static_cast<int>(ip)]);
}

void EndPoint::SetPortNumber(const u_short pNumber)
{
    epData_.sin_port = htons(pNumber);
}


const char* EndPoint::GetIpAddress(const int family)
{
    char buffer[INET6_ADDRSTRLEN];
    const char* result = InetNtop(family, &epData_.sin_addr, buffer, sizeof(buffer));
    if (result == NULL)
    {
        throw std::runtime_error("EndPoint::GetIpAdress(): " + SocketError::getError(WSAGetLastError()));
    }
    return result;
}

u_short EndPoint::GetPortNumber()
{
    return ntohs(epData_.sin_port);
}

void EndPoint::ResetEndPoint()
{
    memset(reinterpret_cast<char *>(&epData_), 0, sizeof(epData_));
}
