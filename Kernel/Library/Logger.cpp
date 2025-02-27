/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Drivers/Serial.hpp>
#include <Drivers/Terminal.hpp>

#include <Library/Logger.hpp>
#include <Library/Spinlock.hpp>

#include <Prism/Debug/LogSink.hpp>
#include <Prism/Math.hpp>
#include <Prism/StringUtils.hpp>

#include <magic_enum/magic_enum.hpp>

namespace E9
{
    CTOS_NO_KASAN static void PrintChar(u8 c)
    {
#if CTOS_ARCH == CTOS_ARCH_X86_64
        __asm__ volatile("outb %0, %1" : : "a"(c), "d"(u16(0xe9)));
#endif
    }

    CTOS_NO_KASAN static void PrintString(std::string_view str)
    {
        for (auto c : str) PrintChar(c);
    }
}; // namespace E9

using namespace Prism;

static usize    s_EnabledSinks = 0;
static Terminal s_Terminal;
class CoreSink  final : public LogSink
{
  public:
    void WriteNoLock(std::string_view str) override
    {
        if (s_EnabledSinks & LOG_SINK_E9) E9::PrintString(str);
        if (s_EnabledSinks & LOG_SINK_SERIAL) Serial::Write(str);
        if (s_EnabledSinks & LOG_SINK_TERMINAL) s_Terminal.PrintString(str);
    }
};
CoreSink g_CoreSink;

namespace Logger
{
    namespace
    {
        Spinlock s_Lock;
        u64      s_LogForegroundColors[] = {
            FOREGROUND_COLOR_WHITE,  FOREGROUND_COLOR_MAGENTA,
            FOREGROUND_COLOR_GREEN,  FOREGROUND_COLOR_CYAN,
            FOREGROUND_COLOR_YELLOW, FOREGROUND_COLOR_RED,
            FOREGROUND_COLOR_WHITE,
        };

        template <typename T>
        CTOS_NO_KASAN void
        LogNumber(va_list& args, int base, bool justifyLeft = false,
                  bool plusSign = false, bool spaceIfNoSign = false,
                  bool zeroPadding = false, usize lengthSpecifier = 0)
        {
            char   buf[64];
            T      value   = va_arg(args, T);
            char*  str     = ToString(value, buf, base);
            size_t len     = strlen(str);
            char   padding = zeroPadding ? '0' : ' ';
            if (plusSign && lengthSpecifier > 0) lengthSpecifier--;
            if (!justifyLeft)
            {
                while (len < lengthSpecifier)
                {
                    LogChar(padding);
                    lengthSpecifier--;
                }
            }
            if (value >= 0)
            {
                if (plusSign) LogChar('+');
                else if (spaceIfNoSign) LogChar(' ');
            }
            while (*str) LogChar(*str++);
            if (justifyLeft)
            {
                while (len < lengthSpecifier)
                {
                    LogChar(padding);
                    lengthSpecifier--;
                }
            }
        }

        CTOS_NO_KASAN void PrintArg(const char*& fmt, va_list& args)
        {
            ++fmt;
            bool leftJustify   = false;
            bool plusSign      = false;
            bool spaceIfNoSign = false;
            bool altConversion = false;
            bool zeroPadding   = false;
            while (true)
            {
                switch (*fmt)
                {
                    case '\0': return;
                    case '-': leftJustify = true; break;
                    case '+': plusSign = true; break;
                    case ' ': spaceIfNoSign = true; break;
                    case '#': altConversion = true; break;
                    case '0': zeroPadding = true; break;

                    default: goto loop_end;
                }
                fmt++;
            }
        loop_end:
            char* numStart  = const_cast<char*>(fmt);
            int   numStrLen = 0;
            while (*fmt >= '0' && *fmt <= '9')
            {
                numStrLen++;
                fmt++;
            }
            int lengthSpecifier = 0;
            if (numStrLen > 0)
                lengthSpecifier = ToNumber<i32>(numStart, numStrLen);
            if (*fmt == '*')
            {
                lengthSpecifier = va_arg(args, i32);
                fmt++;
            }
            enum class ArgLength
            {
                eInt,
                eLong,
                eLongLong,
                eSizeT,
                ePointer,
            };

            ArgLength argLength = ArgLength::eInt;
            if (*fmt == 'l')
            {
                fmt++;
                argLength = ArgLength::eLong;
                if (*fmt == 'l')
                {
                    argLength = ArgLength::eLongLong;
                    fmt++;
                }
            }
            else if (*fmt == 'z')
            {
                argLength = ArgLength::eSizeT;
                fmt++;
            }
            i32 base = 10;

#define LogNum(type)                                                           \
    LogNumber<type>(args, base, leftJustify, plusSign, spaceIfNoSign,          \
                    zeroPadding, lengthSpecifier)
            switch (*fmt)
            {
                case 'b':
                    base = 2;
                    if (altConversion) Print("0b");
                    goto print_unsigned;
                case 'o':
                    base = 8;
                    if (altConversion) LogChar('0');
                    goto print_unsigned;
                case 'p': argLength = ArgLength::ePointer;
                case 'X':
                case 'x':
                    base = 16;
                    if (altConversion)
                    {
                        LogChar('0');
                        LogChar('x');
                    }
                case 'u':
                {
                print_unsigned:
                    if (argLength == ArgLength::eInt) LogNum(unsigned int);
                    else if (argLength == ArgLength::eLong)
                        LogNum(unsigned long);
                    else if (argLength == ArgLength::eLongLong)
                        LogNum(unsigned long long);
                    else if (argLength == ArgLength::eSizeT) LogNum(size_t);
                    else LogNum(uintptr_t);
                    break;
                }
                case 'd':
                case 'i':
                {
                    if (argLength == ArgLength::eInt) LogNum(int);
                    else if (argLength == ArgLength::eLong) LogNum(long long);
                    else if (argLength == ArgLength::eLongLong) LogNum(long);
                    break;
                }
                case 'c': LogChar(*fmt); break;
                case 's':
                {
                    char* str = va_arg(args, char*);
                    while (*str != '\0') LogChar(*str++);
                }
                break;
            }
            ++fmt;
        }

        CTOS_NO_KASAN void PrintLogLevel(LogLevel logLevel)
        {
            if (logLevel == LogLevel::eNone) return;
            Print("[");

            LogChar(s_LogForegroundColors[std::to_underlying(logLevel)]);
            if (logLevel == LogLevel::eFatal) LogChar(BACKGROUND_COLOR_RED);

            Print(magic_enum::enum_name(logLevel).data() + 1);

            LogChar(FOREGROUND_COLOR_WHITE);
            LogChar(BACKGROUND_COLOR_BLACK);
            LogChar(RESET_COLOR);
            Print("]: ");
        }
    }; // namespace

    CTOS_NO_KASAN void EnableSink(usize output)
    {
        if (output == LOG_SINK_TERMINAL)
            s_Terminal.Initialize(*BootInfo::GetFramebuffer());

        s_EnabledSinks |= output;
    }
    CTOS_NO_KASAN void DisableSink(usize output) { s_EnabledSinks &= ~output; }

    void LogChar(u64 c) { Print(reinterpret_cast<const char*>(&c)); }
    void Print(std::string_view string) { g_CoreSink.WriteNoLock(string); }
    i32  Printv(const char* format, va_list* args)
    {
        Logv(LogLevel::eNone, format, *args);

        return 0;
    }

    CTOS_NO_KASAN void Log(LogLevel logLevel, std::string_view string)
    {
        ScopedLock guard(s_Lock, true);

        PrintLogLevel(logLevel);
        Print(string);

        if (logLevel != LogLevel::eNone) LogChar('\n');
    }

    CTOS_NO_KASAN void Logf(LogLevel logLevel, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        Logv(logLevel, fmt, args);
        va_end(args);
    }
    CTOS_NO_KASAN void Logv(LogLevel level, const char* fmt, va_list& args)
    {
        ScopedLock guard(s_Lock, true);
        PrintLogLevel(level);
        while (*fmt)
        {
            if (*fmt == '%') PrintArg(fmt, args);
            else if (*fmt == '\n')
            {
                LogChar('\r');
                LogChar('\n');
                fmt++;
            }
            else LogChar(*fmt++);
        }

        LogChar('\n');
    }

    Terminal& GetTerminal() { return s_Terminal; }
    void      Unlock() { s_Lock.Release(); }
} // namespace Logger
