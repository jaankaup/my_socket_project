#ifndef WINSOCK_H
#define WINSOCK_H

#include "socketHeaders.h"

/*
 * WinSock luokka. Jotta ohjelmassa voi k�ytt�� windowsin socketteja, niin t�ytyy ensin luoda WinSock olio create()-j�senfunktiolla.
 */
class WinSock
{
    public:
        /// Luo singleton WinSock-olion. Ep�onnistuessaan heitt�� poikkeuksen.
        WinSock& create();
        /// Destructori. Vapauttaa k�ytt�j�rjestelm�lt� varatut resurssit.
        virtual ~WinSock();

    protected:

    private:
        /// Rakennnin. Heitt�� poikkeuksen ep�onnistuessaan.
        WinSock();
};

#endif // WINSOCK_H