#ifndef WINSOCK_H
#define WINSOCK_H

#include "socketHeaders.h"

/*
 * WinSock luokka. Jotta ohjelmassa voi k‰ytt‰‰ windowsin socketteja, niin t‰ytyy ensin luoda WinSock olio create()-j‰senfunktiolla.
 */
class WinSock
{
    public:
        /// Luo singleton WinSock-olion. Ep‰onnistuessaan heitt‰‰ poikkeuksen.
        static WinSock& Create();

    protected:

    private:
        /// Rakennin. Heitt‰‰ poikkeuksen ep‰onnistuessaan.
        WinSock();
        /// Destructori. Vapauttaa k‰yttˆj‰rjestelm‰lt‰ varatut resurssit.
        virtual ~WinSock();
};

#endif // WINSOCK_H
