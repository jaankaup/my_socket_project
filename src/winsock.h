#ifndef WINSOCK_H
#define WINSOCK_H

#include "socketHeaders.h"

/*
 * WinSock luokka. Jotta ohjelmassa voi käyttää windowsin socketteja, niin täytyy ensin luoda WinSock olio create()-jäsenfunktiolla.
 */
class WinSock
{
    public:
        /// Luo singleton WinSock-olion. Epäonnistuessaan heittää poikkeuksen.
        WinSock& create();
        /// Destructori. Vapauttaa käyttöjärjestelmältä varatut resurssit.
        virtual ~WinSock();

    protected:

    private:
        /// Rakennnin. Heittää poikkeuksen epäonnistuessaan.
        WinSock();
};

#endif // WINSOCK_H
