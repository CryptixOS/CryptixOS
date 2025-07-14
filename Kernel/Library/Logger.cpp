/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Serial.hpp>
#include <Drivers/VideoTerminal.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Library/Logger.hpp>

#include <Prism/Debug/LogSink.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Math.hpp>

namespace E9
{
    CTOS_NO_KASAN static void PrintChar(u8 c)
    {
#ifdef CTOS_TARGET_X86_64
        __asm__ volatile("outb %0, %1" : : "a"(c), "d"(u16(0xe9)));
#endif
    }

    CTOS_NO_KASAN static void PrintString(StringView str)
    {
        for (auto c : str) PrintChar(c);
    }
}; // namespace E9

using namespace Prism;

static usize   s_EnabledSinks = 0;
class CoreSink final : public LogSink
{
  public:
    void WriteNoLock(StringView str) override
    {
        if (s_EnabledSinks & LOG_SINK_E9) E9::PrintString(str);
        if (s_EnabledSinks & LOG_SINK_SERIAL) Serial::Write(str);
        if (s_EnabledSinks & LOG_SINK_TERMINAL)
        {
            auto terminal = Terminal::GetPrimary();
            if (!terminal) return;
            terminal->PrintString(str);
        }
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

        struct PrintfFormatSpec
        {
            bool  JustifyLeft = false;
            bool  PrintSign   = false;
            bool  Space       = false;
            bool  PrintBase   = false;
            bool  ZeroPad     = false;
            char  Width       = 'd';
            isize Length      = -1;
            u64   Precision   = 999;
        };
        template <typename T>
        CTOS_NO_KASAN void LogNumber(va_list& args, int base,
                                     PrintfFormatSpec& spec)
        {
            T      value   = va_arg(args, T);
            String str     = ToString(value, base);

            char   padding = spec.ZeroPad ? '0' : ' ';
            if (spec.PrintSign && spec.Length > 0) spec.Length--;
            if (!spec.JustifyLeft)
            {
                while (static_cast<isize>(str.Size()) < spec.Length)
                {
                    LogChar(padding);
                    spec.Length--;
                }
            }
            if (value >= 0)
            {
                if (spec.PrintSign) LogChar('+');
                else if (spec.Space) LogChar(' ');
            }
            for (const auto c : str) LogChar(c);
            if (spec.JustifyLeft)
            {
                while (static_cast<isize>(str.Size()) < spec.Length)
                {
                    LogChar(padding);
                    spec.Length--;
                }
            }
        }

        enum class State
        {
            eBegin,
            eFlags,
            eAlign,
            eSign,
            eHash,
            eZero,
            eWidth,
            ePrecision,
            eEnd,
        };
        struct PrintfFormatParser
        {
            constexpr PrintfFormatSpec operator()(const char*& fmt,
                                                  va_list&     args)
            {
                ++fmt;
                PrintfFormatSpec spec;

                struct
                {
                    enum State     State = State::eBegin;
                    constexpr void operator()(enum State state)
                    {
                        if (State >= state) assert("invalid format specifier");
                        State = state;
                    }
                } enterState;

                char* precisionStart = nullptr;
                for (;;)
                {
                    switch (*fmt)
                    {
                        case '\0': return spec;
                        case '-':
                            enterState(State::eFlags);
                            spec.JustifyLeft = true;
                            break;
                        case '+':
                            enterState(State::eFlags);
                            spec.PrintSign = true;
                            break;
                        case ' ':
                            enterState(State::eFlags);
                            spec.Space = true;
                            break;
                        case '#':
                            enterState(State::eFlags);
                            spec.PrintBase = true;
                            break;
                        case '0':
                            enterState(State::eFlags);
                            spec.ZeroPad = true;
                            break;
                        case '.':
                            enterState(State::eFlags);
                            precisionStart = const_cast<char*>(fmt) + 1;
                            break;

                        default: goto loop_end;
                    }
                    fmt++;
                }
            loop_end:

                auto parseNumber = [](const char*& fmt) -> isize
                {
                    char* start  = const_cast<char*>(fmt);
                    usize length = 0;

                    for (; StringUtils::IsDigit(*fmt); length++, fmt++);
                    StringView numberString(start, length);

                    return ToNumber<isize>(numberString, 10);
                };

                isize precision = 1000;
                if (fmt != precisionStart)
                {
                    if (StringUtils::IsDigit(*fmt))
                        spec.Length = parseNumber(fmt);
                    else if (*fmt == '*') spec.Length = va_arg(args, i32);
                }
                if (*fmt == '*' && fmt == precisionStart)
                    precision = va_arg(args, i32);
                else if (StringUtils::IsDigit(*fmt) && fmt == precisionStart)
                    precision = parseNumber(fmt);

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

#define LogNum(type) LogNumber<type>(args, base, spec)
                switch (*fmt)
                {
                    case 'b':
                        base = 2;
                        if (spec.PrintBase) Print("0b");
                        goto print_unsigned;
                    case 'o':
                        base = 8;
                        if (spec.PrintBase) LogChar('0');
                        goto print_unsigned;
                    case 'p': argLength = ArgLength::ePointer;
                    case 'X':
                    case 'x':
                        base = 16;
                        if (spec.PrintBase)
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
                        else if (argLength == ArgLength::eSizeT) LogNum(usize);
                        else LogNum(uintptr_t);
                        break;
                    }
                    case 'd':
                    case 'i':
                    {
                        if (argLength == ArgLength::eInt) LogNum(int);
                        else if (argLength == ArgLength::eLong)
                            LogNum(long long);
                        else if (argLength == ArgLength::eLongLong)
                            LogNum(long);
                        break;
                    }
                    case 'c': LogChar(*fmt); break;
                    case 's':
                    {
                        char* str = va_arg(args, char*);
                        for (; *str != '\0' && precision > 0; --precision)
                            LogChar(*str++);
                    }
                    break;
                }
                ++fmt;
                return spec;
            }
        };

        CTOS_NO_KASAN void PrintLogLevel(LogLevel logLevel)
        {
            if (logLevel == LogLevel::eNone) return;
            Print("[");

            LogChar(s_LogForegroundColors[ToUnderlying(logLevel)]);
            if (logLevel == LogLevel::eFatal) LogChar(BACKGROUND_COLOR_RED);

            auto logLevelString = StringUtils::ToString(logLevel);
            logLevelString.RemovePrefix(1);

            Print(logLevelString);

            LogChar(FOREGROUND_COLOR_WHITE);
            LogChar(BACKGROUND_COLOR_BLACK);
            LogChar(RESET_COLOR);
            Print("]:");

            for (usize i = 0; i < 8 - logLevelString.Size(); i++) LogChar(' ');
        }
    }; // namespace

    CTOS_NO_KASAN void EnableSink(usize output)
    {
        auto terminal = Terminal::GetPrimary();
        if (output == LOG_SINK_TERMINAL && terminal)
            terminal->Initialize(*BootInfo::GetPrimaryFramebuffer());

        s_EnabledSinks |= output;
    }
    CTOS_NO_KASAN void DisableSink(usize output) { s_EnabledSinks &= ~output; }

    void LogChar(u64 c) { Print(reinterpret_cast<const char*>(&c)); }
    void Print(StringView string) { g_CoreSink.WriteNoLock(string); }
    i32  Printv(const char* format, va_list* args)
    {
        Logv(LogLevel::eNone, format, *args);

        return 0;
    }

    CTOS_NO_KASAN void Log(LogLevel logLevel, StringView string,
                           bool printNewline)
    {
        ScopedLock guard(s_Lock, true);

        PrintLogLevel(logLevel);
        Print(string);

        if (printNewline && logLevel != LogLevel::eNone) LogChar('\n');
    }

    CTOS_NO_KASAN void Logf(LogLevel logLevel, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        Logv(logLevel, fmt, args);
        va_end(args);
    }
    CTOS_NO_KASAN void Logv(LogLevel level, const char* fmt, va_list& args,
                            bool printNewline)
    {
        ScopedLock guard(s_Lock, true);
        PrintLogLevel(level);
        while (*fmt)
        {
            if (*fmt == '%')
            {
                PrintfFormatParser parser;
                parser(fmt, args);
            }
            else if (*fmt == '\n')
            {
                LogChar('\r');
                LogChar('\n');
                fmt++;
            }
            else LogChar(*fmt++);
        }

        if (printNewline) LogChar('\n');
    }

    Terminal& GetTerminal() { return *Terminal::GetPrimary(); }
    void      Unlock() { s_Lock.Release(); }
} // namespace Logger
