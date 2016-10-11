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
    std::getline(std::cin,line);
    return line;
}

////////////////////////////////////////////////////////////////////////////

bool KeyPressed()
{
    auto foreWindow = GetForegroundWindow();
    auto thisWindow = GetConsoleWindow();

    // ClearConsoleInputBuffer();

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

bool Startswith(const std::string& prefix, const std::string& source)
{
    // This would be better with regular expressions, but this will do for now.
    if (prefix.length() > source.length()) return false;
    //int maxLength = std::max(s1.length(), s2.length());
    for (int i=0; i<prefix.length(); ++i) if (prefix[i] != source[i]) return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////

bool Endswith(const std::string& postFix, const std::string& source)
{
    // This would be better with regular expressions, but this will do for now.
    int l1 = postFix.length();
    int l2 = source.length();
    for (int i=l2-1, j=l1-1 ; (i>-1 && j>-1) ; --i, --j)
    {
        if (postFix[j] != source[i]) return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////

void TypeText(Socket& s, EndPoint& ep, bool& writing, bool& quit)
{
    using namespace std::chrono;

    std::string data = ReadLine("");
    if (data == "q")
    {
        writing = false;
        quit = true;
        return;
    }

    int status = s.SendTo(data,ep);
    std::this_thread::sleep_for(milliseconds(500));
    KeyPressed();
    writing = false;
}

////////////////////////////////////////////////////////////////////////////

void TypeText2(Socket& s, EndPoint& ep, bool& writing, const std::string& prefix)
{
    using namespace std::chrono;

    std::string data = ReadLine("");
    int status = s.SendTo(prefix + data,ep);
    std::this_thread::sleep_for(milliseconds(500));
    KeyPressed();
    writing = false;
}

////////////////////////////////////////////////////////////////////////////

void ClearConsoleInputBuffer()
{
    std::unique_ptr<INPUT_RECORD> cVar1(new INPUT_RECORD[256]);
    DWORD cVar2;
    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), cVar1.get(),256,&cVar2);
}

////////////////////////////////////////////////////////////////////////////

bool ParseNumber(const std::string& str, int& number)
{
        try
        {
            number = std::stoi(str,nullptr,10);
        }
        catch (std::invalid_argument ex)
        {
            return false;
        }
        return true;
}

////////////////////////////////////////////////////////////////////////////

} // namespace Misc
