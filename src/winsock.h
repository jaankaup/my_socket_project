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
        static WinSock& Create();

    protected:

    private:
        /// Rakennin. Heittää poikkeuksen epäonnistuessaan.
        WinSock();
        /// Destructori. Vapauttaa käyttöjärjestelmältä varatut resurssit.
        virtual ~WinSock();
};

#endif // WINSOCK_H
