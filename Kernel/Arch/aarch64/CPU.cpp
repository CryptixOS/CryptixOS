/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

#include <limine.h>
extern limine_mp_response* SMP_Response();
namespace CPU
{
    namespace
    {
        CPU*      s_BSP      = nullptr;
        usize     s_BspMpIdr = 0;
        CPU::List s_CPUs;
        usize     s_OnlineCPUsCount = 1;
    }; // namespace

    KERNEL_INIT_CODE void InitializeBSP()
    {
        limine_mp_response* smp      = SMP_Response();
        usize               cpuCount = smp->cpu_count;
        s_BspMpIdr                   = smp->bsp_mpidr;

        for (usize i = 0; i < cpuCount; i++)
        {
            limine_mp_info* smpInfo = smp->cpus[i];
            if (smpInfo->mpidr != s_BspMpIdr) continue;

            auto& cpu               = GetCPU(i);
            smpInfo->extra_argument = Pointer(&cpu);
        }
    }

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

    struct CPU;
    CPU*         Current() { return nullptr; }
    CPU*         GetCurrent() { return nullptr; }
    Thread*      GetCurrentThread() { return nullptr; }

    ClockSource* HighResolutionClock() { return nullptr; }

    bool         SwapInterruptFlag(bool) { return false; }

    void         PrepareThread(Thread* thread, Pointer pc, Pointer)
    {
        (void)thread;
        (void)pc;
    }

    void SaveThread(Thread* thread, ExecutionContext* ctx)
    {
        (void)thread;
        (void)ctx;
    }
    void LoadThread(Thread* thread, ExecutionContext* ctx)
    {
        (void)thread;
        (void)ctx;
    }

    void Reschedule(Timestep) {}

    void HaltAll() {}
    void WakeUp(usize, bool) {}

    u64  GetBspId() { return s_BspMpIdr; }
    CPU& GetBsp() { return *s_BSP; }
    CPU& GetCPU(usize id)
    {
        for (usize i = 0; auto cpu : s_CPUs)
        {
            if (i == id) return *cpu;
            ++i;
        }

        AssertNotReached();
    }

    CPU::List& GetCPUs() { return s_CPUs; }
    u64        GetOnlineCPUsCount() { return s_OnlineCPUsCount; }
    u64        GetCurrentID() { return GetCurrent()->ID; }

    bool       DuringSyscall() { return false; }
    void       OnSyscallEnter(usize index) {}
    void       OnSyscallLeave() {}
} // namespace CPU
