/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Logger.hpp"

#include "Drivers/Serial.hpp"

#include <mutex>

namespace E9
{
    static void PrintChar(u8 c)
    {
#if CTOS_ARCH == CTOS_ARCH_X86_64
        __asm__ volatile("outb %0, %1" : : "a"(c), "d"(u16(0xe9)));
#endif
    }
    static void PrintString(std::string_view str)
    {
        for (auto c : str) PrintChar(c);
    }
}; // namespace E9

namespace Logger
{
    static usize      enabledOutputs = 0;
    static std::mutex lock;

    void              EnableOutput(usize output) { enabledOutputs |= output; }
    void              DisableOutput(usize output) { enabledOutputs &= ~output; }

    void              LogChar(u64 c)
    {
        usize len = 1;
        if (c == RESET_COLOR) len = 4;
        else if (c > 255) len = 5;
        LogString(std::string_view(reinterpret_cast<const char*>(&c), len));
        if (c == '\n') LogChar('\r');
    }

    void LogString(std::string_view str)
    {
        if (enabledOutputs & LOG_OUTPUT_E9) E9::PrintString(str);
        if (enabledOutputs & LOG_OUTPUT_SERIAL) Serial::Write(str);
        if (enabledOutputs & LOG_OUTPUT_TERMINAL)
            ; // terminal.PrintString(str.data(), len);
    }

    void Log(LogLevel logLevel, std::string_view string)
    {
        std::unique_lock guard(lock);

        if (logLevel != LogLevel::eNone) LogChar('[');
        switch (logLevel)
        {
            case LogLevel::eDebug:
                LogChar(FOREGROUND_COLOR_MAGENTA);
                LogString("Debug");
                break;
            case LogLevel::eTrace:
                LogChar(FOREGROUND_COLOR_GREEN);
                LogString("Trace");
                break;
            case LogLevel::eInfo:
                LogChar(FOREGROUND_COLOR_CYAN);
                LogString("Info");
                break;
            case LogLevel::eWarn:
                LogChar(FOREGROUND_COLOR_YELLOW);
                LogString("Warn");
                break;
            case LogLevel::eError:
                LogChar(FOREGROUND_COLOR_RED);
                LogString("Error");
                break;
            case LogLevel::eFatal:
                LogChar(BACKGROUND_COLOR_RED);
                LogChar(FOREGROUND_COLOR_WHITE);
                LogString("Fatal");
                break;

            default: break;
        }

        if (logLevel != LogLevel::eNone)
        {
            LogChar(FOREGROUND_COLOR_WHITE);
            LogChar(BACKGROUND_COLOR_BLACK);
            LogChar(RESET_COLOR);
            LogString("]: ");
        }
        LogString(string);
        LogChar('\n');
    }
} // namespace Logger
