#include "misc.h"

namespace Misc
{
    void ReadKey(const std::string& message)
    {
        std::cout << message;
        while (true)
        {
            if (KeyPressed()) break;
        }
    }

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

} // namespace Misc
