#ifndef SOCKETEXCEPTION_H
#define SOCKETEXCEPTION_H

#include <stdexcept>
#include <string>

/*
 * An exception class for Sockets.
 */
class SocketException : public std::runtime_error
{
    public:
        SocketException(const std::string& message) : std::runtime_error(message) {};
        SocketException(const char* message) : std::runtime_error(message) {};
};

class EndPointException : public std::runtime_error
{
    public:
        EndPointException(const std::string& message) : std::runtime_error(message) {};
        EndPointException(const char* message) : std::runtime_error(message) {};
};

#endif // SOCKETEXCEPTION_H
