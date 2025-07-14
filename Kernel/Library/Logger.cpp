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

#include <Prism/String/Printf.hpp>
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

    CTOS_NO_KASAN static isize PrintString(StringView str)
    {
        isize nwritten = 0;
        for (auto c : str) PrintChar(c), ++nwritten;

        return nwritten;
    }
}; // namespace E9

using namespace Prism;

static usize   s_EnabledSinks = 0;
class CoreSink final : public LogSink
{
  public:
    isize WriteNoLock(StringView str) override
    {
        isize nwritten = 0;
        if (s_EnabledSinks & LOG_SINK_E9) nwritten = E9::PrintString(str);
        isize ret = 0;
        if (s_EnabledSinks & LOG_SINK_SERIAL) ret = Serial::Write(str);
        nwritten = nwritten ?: ret;
        if (s_EnabledSinks & LOG_SINK_TERMINAL)
        {
            auto terminal = Terminal::GetPrimary();
            if (!terminal) return nwritten;

            if (nwritten == 0) nwritten = terminal->PrintString(str);
        }

        return nwritten;
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

        using ArgumentType       = Printf::ArgumentType;
        using Sign               = Printf::Sign;
        using PrintfFormatSpec   = Printf::FormatSpec;
        using PrintfFormatParser = Printf::FormatParser;
        template <typename T>
        CTOS_NO_KASAN isize LogNumber(va_list& args, PrintfFormatSpec& spec)
        {
            isize nwritten = 0;
            T     value    = va_arg(args, T);

            if (value < 0 && spec.Base == 10)
            {
                spec.Sign = Sign::eMinus;
                value *= -1;
            }
            String str     = ToString(value, spec.Base);

            char   padding = spec.ZeroPad ? '0' : ' ';
            if (spec.Sign != Sign::eNone && spec.Length > 0) spec.Length--;
            if (!spec.JustifyLeft)
            {
                while (static_cast<isize>(str.Size()) < spec.Length)
                {
                    LogChar(padding);
                    --spec.Length;
                    ++nwritten;
                }
            }

            if (spec.Sign == Sign::ePlus) LogChar('+'), ++nwritten;
            else if (spec.Sign == Sign::eMinus) LogChar('-'), ++nwritten;
            else if (spec.Sign == Sign::eSpace) LogChar(' '), ++nwritten;

            if (spec.Base != 10 && spec.PrintBase)
            {
                if (spec.Base == 2) nwritten += Print("0b");
                else if (spec.Base == 8) nwritten += Print("0");
                else if (spec.Base == 16) nwritten += Print("0x");
            }

            for (const auto c : str)
                LogChar(spec.UpperCase && c >= 'a' && c <= 'z'
                            ? StringUtils::ToUpper(c)
                            : c),
                    ++nwritten;
            while (spec.JustifyLeft
                   && static_cast<isize>(str.Size()) < spec.Length)
            {
                LogChar(padding);
                --spec.Length;
                ++nwritten;
            }

            return nwritten;
        }

        CTOS_NO_KASAN isize PrintLogLevel(LogLevel logLevel)
        {
            isize nwritten = 0;
            if (logLevel == LogLevel::eNone) return nwritten;
            nwritten += Print("[");

            LogChar(s_LogForegroundColors[ToUnderlying(logLevel)]);
            ++nwritten;
            if (logLevel == LogLevel::eFatal)
                LogChar(BACKGROUND_COLOR_RED), ++nwritten;

            auto logLevelString = StringUtils::ToString(logLevel);
            logLevelString.RemovePrefix(1);

            nwritten += Print(logLevelString);

            LogChar(FOREGROUND_COLOR_WHITE);
            LogChar(BACKGROUND_COLOR_BLACK);
            LogChar(RESET_COLOR);

            nwritten += Print("]:") + 3;

            for (usize i = 0; i < 8 - logLevelString.Size(); i++, nwritten++)
                LogChar(' ');

            return nwritten;
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

    void  LogChar(u64 c) { Print(reinterpret_cast<const char*>(&c)); }
    isize Print(StringView string) { return g_CoreSink.WriteNoLock(string); }
    isize Printv(const char* format, va_list* args)
    {
        return Logv(LogLevel::eNone, format, *args);
    }

    CTOS_NO_KASAN isize Log(LogLevel logLevel, StringView string,
                            bool printNewline)
    {
        ScopedLock guard(s_Lock, true);

        isize      nwritten = PrintLogLevel(logLevel);
        nwritten += Print(string);

        if (printNewline && logLevel != LogLevel::eNone)
            LogChar('\n'), ++nwritten;
        return nwritten;
    }

    CTOS_NO_KASAN isize Logf(LogLevel logLevel, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        auto nwritten = Logv(logLevel, fmt, args);
        va_end(args);

        return nwritten;
    }

    isize PrintArgument(StringView::Iterator& it, va_list& args,
                        PrintfFormatSpec& specs)
    {
        isize nwritten = 0;

        switch (specs.Type)
        {
            case ArgumentType::eChar:
            {
                i32 c = va_arg(args, i32);
                LogChar(c);
                ++nwritten;
                break;
            }
            case ArgumentType::eInteger:
                nwritten += LogNumber<int>(args, specs);
                break;
            case ArgumentType::eLong:
                nwritten += LogNumber<long>(args, specs);
                break;
            case ArgumentType::eLongLong:
                nwritten += LogNumber<long long>(args, specs);
                break;
            case ArgumentType::eSize:
                nwritten += LogNumber<isize>(args, specs);
                break;
            case ArgumentType::eUnsignedChar:
            {
                i32 c = va_arg(args, i32);
                LogChar(c);
                ++nwritten;
                break;
            }
            case ArgumentType::eUnsignedInteger:
                nwritten += LogNumber<unsigned int>(args, specs);
                break;
            case ArgumentType::eUnsignedLong:
                nwritten += LogNumber<unsigned long>(args, specs);
                break;
            case ArgumentType::eUnsignedLongLong:
                nwritten += LogNumber<unsigned long long>(args, specs);
                break;
            case ArgumentType::eUnsignedSize:
                nwritten += LogNumber<usize>(args, specs);
                break;
            case ArgumentType::eString:
            {
                StringView string = va_arg(args, const char*);
                usize      size   = string.Size();
                if (specs.Precision > 0
                    && specs.Precision < static_cast<isize>(size))
                    size = static_cast<usize>(specs.Precision);
                if (size == 0) break;

                for (const auto& c : string.Substr(0, size))
                    LogChar(c), ++nwritten;
                break;
            }
            case ArgumentType::eOutWrittenCharCount:
            {
                i32* out = va_arg(args, i32*);
                *out     = nwritten;
                break;
            }

            default: break;
        }

        return nwritten;
    }
    CTOS_NO_KASAN isize Logv(LogLevel level, const char* fmt, va_list& args,
                             bool printNewline)
    {
        ScopedLock guard(s_Lock, true);
        PrintLogLevel(level);

        isize      nwritten = 0;
        auto       it       = fmt;
        while (*it)
        {
            if (*it == '%' && *(it + 1) != '%')
            {
                PrintfFormatParser parser;
                auto   specs = parser(it, args);

                nwritten += PrintArgument(it, args, specs);
                continue;
            }
            else if (*it == '\n')
            {
                LogChar('\r');
                LogChar('\n');
                it++;
                nwritten += 2;
                continue;
            }
            LogChar(*it++), nwritten++;
        }

        if (printNewline) LogChar('\n'), ++nwritten;
        return nwritten;
    }

    Terminal& GetTerminal() { return *Terminal::GetPrimary(); }
    void      Unlock() { s_Lock.Release(); }
} // namespace Logger
