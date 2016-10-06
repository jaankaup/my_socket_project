#ifndef SOCKET_H
#define SOCKET_H

#include <stdexcept>
#include <functional>
#include <chrono>
#include <thread>
#include <string>
#include <memory>
#include <iostream>
#include <assert.h>
#include <vector>
#include "socketHeaders.h"
#include "socket.h"
#include "error.h"
#include "winsock.h"
#include "endpoint.h"
#include "socketexception.h"
#include "misc.h"

// Comment this to disable debugging.
#define NDEBUG

// Forward declarations.
class EndPoint;
class SocketExepction;

/// The default buffer length, port and receive/send time out.
constexpr unsigned int DEFAULT_BUFLEN = 1028;
constexpr int DEFAULT_PORT = 25000;
constexpr int DEFAULT_RECEIVE_TIMEOUT = 0;
constexpr int DEFAULT_SEND_TIMEOUT = 0;

/// Socket families. Now only AF_INET is provided.
enum class SocketFamily : int {SOCKET_AF_INET = AF_INET};

/// Socket types. Only these 2 are provided.
enum class SocketType : int {SOCKET_STREAM = SOCK_STREAM,
                             SOCKET_DGRAM = SOCK_DGRAM};

/// Socket protocols. Only TCP and UDP are available.
enum class SocketProtocol: int {SOCKET_TCP = IPPROTO_TCP,
                                SOCKET_UDF = IPPROTO_UDP};

/// An enumeration for socket status. This should be private.
enum class SocketStatus {READ,WRITE /*, ERROR = 2*/};

/*
 * A simple socket class.
 * Author Janne Kauppinen 2016.
 * None right reserved.
 * The implementation of this class is incomplete. This project is just an exercise. Do not use in real programs.
 */
class Socket
{
    public:
        /// Constructor. Throws SocketException on failure.
        Socket(const SocketFamily sf, const SocketType st, const SocketProtocol sp);

        /// Destructor.
        virtual ~Socket();

        /// Set timout for Receive function.
        int GetReceiveTimeout() const;

        /// Set timout for Receive function.
        void SetReceiveTimeout(const int mm);

        /// Set timout for Receive function.
        int GetSendTimeout() const;

        /// Set timout for Receive function.
        void SetSendTimeout(const int mm);

        /// Gets a value that indicates whether the Socket is in blocking mode.
        bool IsBlocking() const;

        unsigned int GetReceiveBufferSize() const;
        void SetReceiveBufferSize(const unsigned int recSize);

        /// Luo yhteyden is‰nt‰‰n. Aiheuttaa runtime_error:in, jos ep‰onnistuu.
        void Connect(const std::string& address, const int port);

        /// Sulkee socketin ja vapauttaa socketin k‰ytt‰m‰t resurssit. Aiheuttaa runtime_error:in, jos
        /// sulkeminen ep‰onnistuu.
        void Close();

        /// Places a Socket in a listening state. @backlog is the maximum length of the pending connections queue.
        /// Throws an SocketException if this socket is closed or in not bound.
        void Listen(const int backlog);

        /// Ottaa vastaan dataa socket:n kautta. Functio palauttaa vastaanotettujen tavujen m‰‰r‰n. @data on
        /// merkkijono johon asetetaan saapuva data.
        int Receive(std::string& data);

        int ReceiveFrom(std::string& s, EndPoint& remote);

        int Send(const std::string& data);

        int SendTo(const std::string& data, EndPoint& remote);

        /// Bind function. Throws SocketException on failure.
        void Bind(const EndPoint& ep);

        /// Accept funktion. To call this function, socket must be successfully bound.
        /// If Accept succeeds, an pointer to a socket is returned for for a newly created connection.
        /// Otherwise an SocketExcpetion is thrown.
        std::unique_ptr<Socket> Accept();

        /// Set the blocking status for the socket..
        void SetNonBlockingStatus(bool status);

        /// Return the local end point. Throw SocketException if the socket is not bound.
        EndPoint GetLocalEndPoint() const;

        /// Return the remote end point. Throw SocketException if the socket is closed or not connected (see. Accpet, Connect).
        EndPoint GetRemoteEndPoint() const;

        /// Gets the amount of data that has been received from the network and is available to be read.
        int Available();

    protected:

    private:
        /// The actual socket.
        SOCKET mSocket = INVALID_SOCKET;

        /// Local end point.
        EndPoint mLocalEndPoint;

        /// Remote end point.
        EndPoint mRemoteEndPoint;

        /// A value that indicates whenever socket is binded.
        bool mIsBound = false;

        /// A value that indicates the connection status of the socket.
        bool mConnected = false;

        /// The maximum length of the pending listening queue.
        int mBacklog = 0;

        /// The size of the send buffer.
        int mSendBufferLength;

        /// The size of the receive buffer.
        int mReceiveBufferLength;

        /// A value that indicates the time to wait until break the Receive-function.
        int mReceiveTimeout;

        /// A value that indicates the time to wait until break the Send-function.
        int mSendTimeout;

        /// A value which indicates the blocking status of the socket.
        bool mIsBlocking;

        /// A struct for basic information of the socket.
        struct SocketInfo
        {
            SocketFamily socketFamily;
            SocketType socketType;
            SocketProtocol socketProtocol;
        } mSocketInfo;

        /// Creates a addrinfo from mSocketInfo.
        addrinfo CreateHints() const;

        /// Creates the private member mSocket.
        void CreateSocket();

        /// Palauttaa socketin input-queue:ssa olevien tavujen lukum‰‰r‰n eli tavujen m‰‰r‰n mit‰ sill‰ hetkell‰
        /// voidaan lukea. @mm_seconds on maksimi aika millisekunneissa jonka t‰m‰ j‰senfunktio saa aikaa suoritukseen.
        /// N‰in v‰ltet‰‰n loputon odottelu jos tavuja ei tulekaan luettavaksi ollenkaan.
        int GetAvailableBytes(const unsigned int mm_seconds);

        /// Tarkistaa socketin tilan. Jos status = SocketStatus::READ niin funktio palauttaa true jos socket on valmis ottamaan dataa vastaan. Muutoni palauttaa false.
        /// Jos taas satus = SocketStatus::WRITE niin funktio palauttaa true jos socket on valmis l‰hett‰m‰‰n dataa. Muutoin palauttaa false.
        /// @timeLimitMilliSec_msec on aika millisekunteina, joka function odottaa saadakseen vastauksen.
        bool CheckStatus(const int timeLimitMilliSec, const SocketStatus status);

};

#endif // SOCKET_H
