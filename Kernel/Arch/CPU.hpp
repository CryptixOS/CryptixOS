/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#ifdef CTOS_TARGET_X86_64
    #include <Arch/x86_64/CPU.hpp>
#elifdef CTOS_TARGET_AARCH64
    #include <Arch/aarch64/CPU.hpp>
#endif

#include <Memory/VMM.hpp>

#include <Prism/Memory/Memory.hpp>

#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/Time.hpp>

class ClockSource;
struct Thread;
struct ExecutionContext;
namespace CPU
{
    constexpr usize KERNEL_STACK_SIZE = 64_kib;
    constexpr usize USER_STACK_SIZE   = 2_mib;

    bool            GetInterruptFlag();
    void            SetInterruptFlag(bool enabled);
    bool            SwapInterruptFlag(bool enabled);

    u64             GetOnlineCPUsCount();
    struct CPU;
    CPU*         Current();
    u64          GetCurrentID();
    CPU*         GetCurrent();

    ClockSource* HighResolutionClock();

    Thread*      GetCurrentThread();

    void         PrepareThread(Thread* thread, Pointer pc, Pointer arg = 0);

    void         SaveThread(Thread* thread, ExecutionContext* ctx);
    void         LoadThread(Thread* thread, ExecutionContext* ctx);

    void         Reschedule(Timestep us);

    void         HaltAll();
    void         WakeUp(usize id, bool everyone);

    bool         DuringSyscall();
    void         OnSyscallEnter(usize index);
    void         OnSyscallLeave();
}; // namespace CPU
