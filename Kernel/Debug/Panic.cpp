/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

#include <Common.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

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
[[noreturn]]
void HaltAndCatchFire(CPUContext* context)
{
    EarlyLogFatal("CPU[%d]: Halted", CPU::GetCurrentID());

    ++s_HaltedCPUs;
    if (s_HaltedCPUs == CPU::GetOnlineCPUsCount())
        EarlyLogFatal("System Halted!\n");

    for (;;) Arch::Halt();
    AssertNotReached();
}

CTOS_NO_KASAN [[noreturn]]
void panic(StringView msg)
{
    enterPanicMode();
    EarlyLogError("Error Message: %s\n", msg.Raw());

    Stacktrace::Print(16);
    EarlyLogFatal("CPU[%d]: Halted", CPU::GetCurrentID());

    s_HaltedCPUs++;
    CPU::HaltAll();
    for (;;) Arch::Halt();
}
CTOS_NO_KASAN [[noreturn]]
void earlyPanic(const char* format, ...)
{
    enterPanicMode();
    Stacktrace::Print(32);

    va_list args;
    va_start(args, format);
    Logger::Logv(LogLevel::eError, format, args);
    va_end(args);

    EarlyLogFatal("CPU[%d]: Halted", CPU::GetCurrentID());

    s_HaltedCPUs++;
    CPU::HaltAll();
    for (;;) Arch::Halt();
}
