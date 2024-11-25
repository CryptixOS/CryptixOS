/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Logger.hpp"

#include "Drivers/Serial.hpp"
#include "Drivers/Terminal.hpp"

#include "Utility/Math.hpp"

#include <mutex>

namespace E9
{
    CTOS_NO_KASAN static void PrintChar(u8 c)
    {
#if CTOS_ARCH == CTOS_ARCH_X86_64
        __asm__ volatile("outb %0, %1" : : "a"(c), "d"(u16(0xe9)));
#endif
    }

    CTOS_NO_KASAN static void PrintString(const char* str)
    {
        while (*str) PrintChar(*str++);
    }
}; // namespace E9

namespace Logger
{
    namespace
    {
        usize      enabledOutputs = 0;
        std::mutex lock;
        Terminal   terminal;

        template <typename T>
        CTOS_NO_KASAN T ToNumber(const char* str, usize length)
        {
            T     integer      = 0;
            bool  isNegative   = str[0] == '-';

            usize index        = isNegative;
            usize stringLength = length, power = stringLength - isNegative;

            for (; index < stringLength; index++)
                integer += (str[index] - 48) * Math::PowerOf<T>(10, --power);

            return (isNegative) ? -integer : integer;
        }
        template <typename T>
        CTOS_NO_KASAN T ToNumber(const char* str)
        {
            return ToNumber<T>(str, strlen(str));
        }

        template <typename T>
        CTOS_NO_KASAN char* ToString(T value, char* str, i32 base)
        {
            T    i          = 0;
            bool isNegative = false;

            if (value == 0)
            {
                str[i++] = '0';
                str[i]   = 0;
                return str;
            }

            if (value < 0 && base == 10)
            {
                isNegative = true;
                value      = -value;
            }

            while (value != '\0')
            {
                T rem    = value % base;
                str[i++] = (rem > 9) ? static_cast<char>((rem - 10) + 'a')
                                     : static_cast<char>(rem + '0');
                value    = value / base;
            }

            if (isNegative) str[i++] = '-';
            str[i]  = '\0';

            T start = 0;
            T end   = i - 1;
            while (start < end)
            {
                char c         = *(str + start);
                *(str + start) = *(str + end);
                *(str + end)   = c;
                start++;
                end--;
            }

            return str;
        }

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
                    if (altConversion) LogString("0b");
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
        }
    }; // namespace

    CTOS_NO_KASAN void EnableOutput(usize output)
    {
        enabledOutputs |= output;

        if (output == LOG_OUTPUT_TERMINAL)
            terminal.Initialize(*BootInfo::GetFramebuffer());
    }
    CTOS_NO_KASAN void DisableOutput(usize output)
    {
        enabledOutputs &= ~output;
    }

    CTOS_NO_KASAN void LogChar(u64 c)
    {
        LogString(reinterpret_cast<const char*>(&c));
        if (c == '\n') LogChar('\r');
    }

    CTOS_NO_KASAN void LogString(const char* str)
    {
        if (enabledOutputs & LOG_OUTPUT_E9) E9::PrintString(str);
        if (enabledOutputs & LOG_OUTPUT_SERIAL) Serial::Write(str);
        if (enabledOutputs & LOG_OUTPUT_TERMINAL) terminal.PrintString(str);
    }

    CTOS_NO_KASAN void Log(LogLevel logLevel, const char* string)
    {
        std::unique_lock guard(lock);

        PrintLogLevel(logLevel);
        LogString(string);
        LogChar('\n');
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
        std::unique_lock guard(lock);
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
} // namespace Logger
