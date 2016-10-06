#include "endpoint.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EndPoint::EndPoint()
{
    mEndPointData = CreateSockaddrin();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EndPoint::EndPoint(const std::string& address, int port)
{
    mEndPointData = CreateSockaddrin();
    SetIpAddress(address);
    SetPortNumber(port);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EndPoint::~EndPoint()
{
    //dtor
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EndPoint::SetInterface(const Interfaces i)
{
    switch(i)
    {
    case Interfaces::ANY :
        SetIpAddress("0.0.0.0");
        break;
    case Interfaces::LOOPBACK :
        SetIpAddress("127.0.0.1");
        break;
    }
}

/*
void EndPoint::SetInterface(const Interfaces interface)
{

    switch(interface) {

    case Interfaces::ANY :
        SetIpAddress("0.0.0.0");
        break;

    case Interfaces::LOOPBACK :
        SetIpAddress("127.0.0.1");
        break;
    }

}
*/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EndPoint::operator==(const EndPoint& other)
{
    if ((mEndPointData.sin_family == other.mEndPointData.sin_family) &&
        (mEndPointData.sin_port   == other.mEndPointData.sin_port) &&
        (mEndPointData.sin_addr.s_addr == other.mEndPointData.sin_addr.s_addr))
    {
        return true;
    }
    else return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EndPoint::SetIPFamily(const IPversion version)
{
    switch(version) {

    case IPversion::IPV4 :
        // Nothing to do right now.
        break;

    case IPversion::IPV6 :
        throw EndPointException("IPV6 not supported yet!");
    };
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EndPoint::SetPortNumber(const int portNumber)
{
    bool limitError = portNumber > std::numeric_limits<u_short>::max() || portNumber < 0;
    if (limitError)
    {
        throw EndPointException("EndPoint::SetPort: portNumber (" + std::to_string(portNumber) + ") is illegal.");
    }

    mEndPointData.sin_port = htons(static_cast<u_short>(portNumber));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int EndPoint::GetPortNumber() const
{
    return ntohs(mEndPointData.sin_port);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string EndPoint::GetIpAddress() const
{
    char buffer[INET6_ADDRSTRLEN];
    const char* result;

    switch (mCurrentVersion)
    {

    case IPversion::IPV4 :
    {
        result = InetNtop(AF_INET,(void*)(&mEndPointData.sin_addr), buffer, sizeof(buffer));
        if (result == NULL)
        {
            throw EndPointException("EndPoint::GetIpAddress: InetNtop - " + SocketError::getError(WSAGetLastError()));
        }
        break;
    }

    case IPversion::IPV6 :
    {
        throw EndPointException("IPV6 not supported yet!");
    }

    }  // switch

    std::string toString(result);
    return toString;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IPversion EndPoint::GetIPFamily() const
{
    return mCurrentVersion;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EndPoint::SetIpAddress(const std::string& address)
{
    if (address.length() == 0)
        throw EndPointException("EndPoint::SetIpAddress: address == \"\". You must give an non-empty string.");

    int result;

    switch (mCurrentVersion)
    {
    case IPversion::IPV4 :
        result = InetPton(mEndPointData.sin_family, address.c_str(), &mEndPointData.sin_addr);
        if (result == 0) throw EndPointException("EndPoint::SetIpAddress: Given address (" + address + ") is malformed.");
        else if (result == -1)
        {
            auto error = WSAGetLastError();
            std::string errorMessage;
            if (error == WSAEAFNOSUPPORT) throw EndPointException("EndPoint::SetIpAddress: InetPton() - The address family "
                                                                  "wasn't supported. Must be AF_INET or AF_INET6.");
            else if (error == WSAEFAULT)
                throw EndPointException("EndPoint::SetIpAddress: InetPton() - The pszAddrString (" + address + ") or pAddrBuf "
                                        "parameters are NULL or are not part of the user address space.");
        }
        else return;

    case IPversion::IPV6 :
        throw std::runtime_error("EndPoint::SetIpAddress: IPV6 not implemented yet!");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sockaddr_in EndPoint::CreateSockaddrin() const
{
    // Lets create sockaddr_in.
    sockaddr_in temp;

    // Reset temp.
    memset(reinterpret_cast<char *>(&temp),0,sizeof(temp));

    // Set IPV4-flag.
    temp.sin_family = AF_INET;

    return temp;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sockaddr EndPoint::ConvertToSockaddr() const
{
    //sockaddr_in temp = mEndPointData;
    //return static_cast<sockaddr>(temp);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
