#ifndef ERROR_H
#define ERROR_H

#include <string>
#include "socketHeaders.h"

namespace SocketError
{
    /// Virheentulostus funktio. Ottaa parametrina WAS-virhekoodin ja palauttaa virheilmoituksen merkkijonona.
    extern std::string getError(const int error);
}

#endif // ERROR_H
