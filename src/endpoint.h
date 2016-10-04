#ifndef EndPoint_H
#define EndPoint_H

#include <limits>
#include "socketHeaders.h"
#include "socketexception.h"
#include "error.h"

enum class Interfaces : int {LOOPBACK, ANY};
enum class IPversion  {IPV4,IPV6};

/*
 * EndPoint class. IPV6 is not implemented yet!
 * Copyright Janne Kauppinen 2016.
 * None rights reserved.
 */
class EndPoint
{
    public:

        /// Constructor.
        EndPoint();

        /// Destructor.
        virtual ~EndPoint();

        /// Equal to operator.
        bool operator==(const EndPoint& other);

        /// Set an IP-address. Throws an EndPointException on failure.
        void SetIpAddress(const std::string& address);

        /// Sets an interface. Throws an EndPointException on failure.
        void SetInterface(const Interfaces v);

        /// Returns the IP-address. Throws an EndPointException on failure
        std::string GetIpAddress() const;

        /// Set an IP-version. Throws an EndPointException on failure.
        void SetIPFamily(const IPversion version);

        /// Returns the IP-family.
        IPversion GetIPFamily() const;

        /// Set a port number. Throws an EndPointException on failure
        void SetPortNumber(const int port);

        /// Returns the port number. Throws an EndPointException on failure
        int GetPortNumber() const;

        /// Creates an addrinfo object. NOT IMPLEMENTED.
        addrinfo CreateAddrinfo() const;

        /// Creates an sockaddr object from this EndPoint. NOT IMPLEMENTED.
        sockaddr ConvertToSockaddr() const;

    private:

        /// IP-adress and port number for IPV4.
        sockaddr_in mEndPointData;

        /// The current version of IP-address.
        IPversion mCurrentVersion = IPversion::IPV4;

        /// Creates a new sockaddr_in object with @ipVersion parameter.
        sockaddr_in CreateSockaddrin() const;
};

#endif // EndPoint_H
