#ifndef ENDPOINT_H
#define ENDPOINT_H
#include <iostream>
#include "socketHeaders.h"

constexpr int IPs[2] = {INADDR_LOOPBACK, INADDR_ANY};
/// LOOP_BACK -> 127.0.0.1 ANY -> 0.0.0.0
enum class IPAddress { LOOP_BACK = 0, ANY = 1};


class EndPoint
{
    public:
        EndPoint();
        ~EndPoint();

        EndPoint& operator=(const EndPoint& other);

        bool operator==(const EndPoint& other);

        /* Toimii nyt mahdollisesti vaarallinen pitemm‰ll‰ t‰ht‰imell‰ katsottuna. Pit‰isi ehk‰ k‰ytt‰‰
         * smart-pointereita. Palauttaa pointerin epData_:an.
         */
        sockaddr_in* operator()();

        /* Vain ip4. */
        const char* GetIpAddress(const int family = AF_INET);
        /* Vain ip4. */
        u_short GetPortNumber();

        void SetIpAddress(int family, const std::string& ipAddress);
        void SetIpAddress(int family, const IPAddress ip);
        void SetPortNumber(const u_short pNumber);

    private:
        sockaddr_in epData_;


        void ResetEndPoint();
};

#endif // ENDPOINT_H

