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
#include <API/Posix/linux/capability.h>
#include <API/UnixTypes.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
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
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>
#include <Prism/Memory/RefCounted.hpp>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

module Cryptix:Impl.Debug.Panic_;

import :Arch.CPU;
import :Debug.Assertions;
import :Library.Logger;
import :Library.Stacktrace;
import :Scheduler.Process;
import :Scheduler.Thread;
import :System.InterruptHandler;

CTOS_NO_KASAN void dumpProcessInfo()
{
    Process* current = Process::Current();
    if (!current) return;

    EarlyLogInfo("Process: %s", current->Name().Raw());
}

inline static void enterPanicMode()
{
    // TODO(v1tr10l7): Halt all cpus
    CPU::SetInterruptFlag(false);
    Logger::Unlock();
    Logger::EnableSink(LOG_SINK_TERMINAL);
    EarlyLogError("Kernel Panic!");
    dumpProcessInfo();
}

static Atomic<u64> s_HaltedCPUs = 0;
CTOS_NORETURN
IrqResult HaltAndCatchFire(Device*, ExecutionContext* context)
{
    EarlyLogFatal("CPU[%d]: Halted", CPU::GetCurrentID());

    ++s_HaltedCPUs;
    if (s_HaltedCPUs == CPU::GetOnlineCPUsCount())
        EarlyLogFatal("System Halted!\n");

    for (;;) Arch::Halt();
    AssertNotReached();
}

extern "C" CTOS_NO_KASAN CTOS_NORETURN void CryptixKernelPanic(const char* message,
                                                                usize       size)
{
    StringView msg(message, size);
    enterPanicMode();
    EarlyLogError("Error Message: %s\n", msg.Raw());

    Stacktrace::Print(16);
    EarlyLogFatal("CPU[%d]: Halted", CPU::GetCurrentID());

    s_HaltedCPUs++;
    CPU::HaltAll();
    for (;;) Arch::Halt();
}
CTOS_NO_KASAN CTOS_NORETURN void earlyPanic(const char* format, ...)
{
    enterPanicMode();
    Stacktrace::Print(32);

    va_list args;
    va_start(args, format);
    Log::Logv(LogLevel::eError, format, args);
    va_end(args);

    EarlyLogFatal("CPU[%d]: Halted", CPU::GetCurrentID());
    s_HaltedCPUs++;
    CPU::HaltAll();
    for (;;) Arch::Halt();
}
