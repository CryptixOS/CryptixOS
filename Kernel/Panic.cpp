/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Arch/CPU.hpp>
#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

CTOS_NO_KASAN void logProcessState()
{
    Process* currentProcess = Process::GetCurrent();
    if (!currentProcess) return;

    EarlyLogInfo("Process: %s", currentProcess->m_Name.data());
}

inline static void enterPanicMode()
{
    // TODO(v1tr10l7): Halt all cpus
    CPU::SetInterruptFlag(false);
    Logger::Unlock();
    Logger::EnableSink(LOG_SINK_TERMINAL);
    EarlyLogError("Kernel Panic!");
    logProcessState();
}
[[noreturn]]
inline static void hcf()
{
    EarlyLogFatal("System Halted!\n");

    Arch::Halt();
    AssertNotReached();
}

CTOS_NO_KASAN [[noreturn]]
void panic(std::string_view msg)
{
    enterPanicMode();
    EarlyLogError("Error Message: %s\n", msg.data());

    Stacktrace::Print(32);
    hcf();
}
CTOS_NO_KASAN [[noreturn]]
void earlyPanic(const char* format, ...)
{
    enterPanicMode();

    va_list args;
    va_start(args, format);
    Logger::Logv(LogLevel::eError, format, args);
    va_end(args);

    Stacktrace::Print(32);
    hcf();
}
