#include "misc.h"

namespace Misc
{

////////////////////////////////////////////////////////////////////////////

void ReadKey(const std::string& message)
{
    std::cout << message;
    while (true)
    {
        if (KeyPressed()) break;
    }
}

////////////////////////////////////////////////////////////////////////////

std::string ReadLine(const std::string& message)
{
    std::string line;
    std::cout << message;
    std::cin >> line;
    return line;
}

////////////////////////////////////////////////////////////////////////////

bool KeyPressed()
{
    auto foreWindow = GetForegroundWindow();
    auto thisWindow = GetConsoleWindow();

    bool pressed = false;

    if (foreWindow == thisWindow)
    {
        for (int i=0; i < 256; ++i)
        {
            if (GetAsyncKeyState(i) & 0x7FFF)
            {
                pressed = true;
                break;
            }
        }
    }
    return pressed;
}

////////////////////////////////////////////////////////////////////////////

extern std::vector<std::string> Tokenize(const std::string& str, const std::string& delimiter)
{
    std::vector<std::string> temp;

    std::regex ws(delimiter);
    std::copy(std::sregex_token_iterator(begin(str),end(str), ws, -1),
              std::sregex_token_iterator(),
              std::back_inserter(temp));

    return temp;
}

////////////////////////////////////////////////////////////////////////////


} // namespace Misc
