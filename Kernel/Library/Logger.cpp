/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/RingBuffer.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Debug/LogSink.hpp>

module Cryptix:Impl.Library.Logger;

import :Drivers.KernelMessageDevice;
import :Drivers.Serial;
import :Drivers.TTY.VirtualConsole;
import :Library.Locking.Spinlock;
import :Library.Logger;

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

// Forwarded to by Logger::Print in Library/PrismHooks.cpp
extern "C" isize CryptixLoggerPrint(const char* message, usize size)
{
    return Logger::Print(StringView(message, size));
}
