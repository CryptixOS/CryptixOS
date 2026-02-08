/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/KernelMessageDevice.hpp>
#include <Drivers/Serial.hpp>
#include <Drivers/TTY/VirtualConsole.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Library/Logger.hpp>

#include <Prism/Debug/LogSink.hpp>

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

static usize s_EnabledSinks = 0;
class CoreSink final : public LogSink<SpinLockPolicy>
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
            auto terminal = VirtualConsole::GetPrimary();
            if (!terminal) return nwritten;

            if (nwritten == 0) nwritten = terminal->PrintString(str);
        }
        KernelMessageDevice::Write(str);

        return nwritten;
    }
};

namespace Logger
{
    namespace
    {
        Spinlock s_Lock;
        CoreSink g_CoreSink;
        // LogLevel s_CurrentLogLevel = LogLevel::eDebug;
    } // namespace

    CTOS_NO_KASAN void EnableSink(usize output)
    {
        auto terminal = VirtualConsole::GetPrimary();
        if (output == LOG_SINK_TERMINAL && terminal)
            terminal->Initialize(VirtualConsole::PrimaryFramebuffer());

        s_EnabledSinks |= output;
    }
    CTOS_NO_KASAN void DisableSink(usize output) { s_EnabledSinks &= ~output; }
    isize Print(StringView string) { return g_CoreSink.WriteNoLock(string); }

    VirtualConsole& GetTerminal() { return *VirtualConsole::GetPrimary(); }
    void            Unlock() { s_Lock.Release(); }
} // namespace Logger
