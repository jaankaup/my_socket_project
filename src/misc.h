#ifndef MISC_H
#define MISC_H

#include <iostream>
#include <vector>
#include <iterator>
#include <regex>
#include <thread>
#include <chrono>
#include <memory>
#include "socketHeaders.h"
#include "socket.h"
#include "endpoint.h"
//#include <vector>
//#include <list>

// Forward declaration.
class Socket;

namespace Misc
{
    /// A function that prints the given @message and waits until the user presses a key.
    extern void ReadKey(const std::string& message);

    /// A functio that prints the given @message and returns a string from console input stream.
    extern std::string ReadLine(const std::string& message);

    /// A function that returns true if a key is pressed on the active console window, or false other wise.
    extern bool KeyPressed();

    /// A function that takes a string @str and a regular expression @delimiter and splits the given string with the
    /// expression. The result is a vector of tokenized strings.
    extern std::vector<std::string> Tokenize(const std::string& str, const std::string& delimiter);

    /// Returns true if the given strings ends with the specified chars, false otherwise.
    extern bool Startswith(const std::string& prefix, const std::string& source);

    /// Returns true if the given strings ends with the specified chars, false otherwise.
    extern bool Endswith(const std::string& postFix, const std::string& source);

    /// A function for ITKP104 programming task.
    extern void TypeText(Socket& s, EndPoint& ep, bool& writing, bool& quit);

    /// A function for ITKP104 programming task.
    extern void TypeText2(Socket& s, EndPoint& ep, bool& writing, const std::string& prefix);

    /// Clear the input buffer of the console.
    extern void ClearConsoleInputBuffer();

    /// Parses a int from a string. Returns true if conversion is succeed, false otherwise.
    extern bool ParseNumber(const std::string& str, int& number);

} //namespace Misc

#endif // MISC_H
