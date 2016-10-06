#ifndef MISC_H
#define MISC_H

#include <iostream>
#include <vector>
#include <iterator>
#include <regex>
#include "socketHeaders.h"
//#include <vector>
//#include <list>

namespace Misc
{
    /// A function that prints the given @message and waits until the user presses a key.
    extern void ReadKey(const std::string& message);

    /// A functio that prints hte given @message and returns a string from console input stream.
    extern std::string ReadLine(const std::string& message);

    /// A function that returns true if a key is pressed on the active console window, or false other wise.
    extern bool KeyPressed();

    /// A function that takes a string @str and a regular expression @delimiter and splits the given string with the
    /// expression. The result is a vector of tokenized strings.
    extern std::vector<std::string> Tokenize(const std::string& str, const std::string& delimiter);

} //namespace Misc

#endif // MISC_H
