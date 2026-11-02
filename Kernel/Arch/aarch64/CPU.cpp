/*
 * Created by v1tr10l7 on 25.11.2024.
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
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
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
#include <limine.h>

module Cryptix:Impl.Arch.aarch64.CPU;

import :Arch.CPU;

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
