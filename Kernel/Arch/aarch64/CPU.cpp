/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

namespace CPU
{
    bool GetInterruptFlag()
    {
        u64 daif = 0;
        __asm__ volatile("mrs %0, daif" : "=r"(daif));

        return daif == 0;
    }
    void SetInterruptFlag(bool enabled)
    {
        if (enabled) __asm__ volatile("msr daifclr, #0b1111");
        else __asm__ volatile("msr daifset, #0b1111");
    }

    usize GetOnlineCPUsCount() { return 1; }

    struct CPU;
    CPU*         Current() { return nullptr; }
    CPU*         GetCurrent() { return nullptr; }
    u64          GetCurrentID() { return 0; }
    Thread*      GetCurrentThread() { return nullptr; }

    ClockSource* HighResolutionClock() { return nullptr; }

    bool         SwapInterruptFlag(bool) { return false; }

    void         PrepareThread(Thread* thread, Pointer pc, Pointer)
    {
        (void)thread;
        (void)pc;
    }

    void SaveThread(Thread* thread, CPUContext* ctx)
    {
        (void)thread;
        (void)ctx;
    }
    void LoadThread(Thread* thread, CPUContext* ctx)
    {
        (void)thread;
        (void)ctx;
    }

    void Reschedule(Timestep) {}

    void HaltAll() {}
    void WakeUp(usize, bool) {}

    UserMemoryProtectionGuard::UserMemoryProtectionGuard() {}
    UserMemoryProtectionGuard::~UserMemoryProtectionGuard() {}

    bool DuringSyscall() { return false; }
    void OnSyscallEnter(usize index) {}
    void OnSyscallLeave() {}
} // namespace CPU
