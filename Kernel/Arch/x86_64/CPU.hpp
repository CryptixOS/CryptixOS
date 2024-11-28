/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Arch/CPU.hpp"

#include "Arch/x86_64/CPUContext.hpp"
#include "Arch/x86_64/GDT.hpp"
#include "Arch/x86_64/IDT.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"

#include <cerrno>
#include <vector>

struct Thread;
namespace CPU
{
    namespace MSR
    {
        constexpr usize IA32_APIC_BASE           = 0x1b;
        constexpr usize IA32_APIC_BASE_BSP       = Bit(8);
        constexpr usize APIC_ENABLE_X2APIC       = Bit(10);
        constexpr usize IA32_APIC_GLOBAL_ENABLE  = Bit(11);

        constexpr usize IA32_PAT                 = 0x277;
        constexpr usize IA32_PAT_RESET           = 0x0007040600070406;

        constexpr usize IA32_EFER                = 0xc0000080;
        constexpr usize STAR                     = 0xc0000081;
        constexpr usize LSTAR                    = 0xc0000082;
        constexpr usize CSTAR                    = 0xc0000083;
        constexpr usize SFMASK                   = 0xc0000084;

        constexpr usize FS_BASE                  = 0xc0000100;
        constexpr usize GS_BASE                  = 0xc0000101;
        constexpr usize KERNEL_GS_BASE           = 0xc0000102;

        constexpr usize IA32_EFER_SYSCALL_ENABLE = Bit(0);
    }; // namespace MSR

    struct CPU
    {
        usize            id;
        void*            empty = nullptr;

        uintptr_t        threadStack;
        uintptr_t        kernelStack;

        u64              lapicID;
        bool             isOnline = false;
        TaskStateSegment tss;

        errno_t          error;
        Thread*          idle;
        Thread*          currentThread;
    } __attribute__((packed));

    void      InitializeBSP();

    bool      ID(u64 leaf, u64 subleaf, u64& rax, u64& rbx, u64& rcx, u64& rdx);
    void      Halt();

    u64       ReadMSR(u32 msr);
    void      WriteMSR(u32 msr, u64 value);

    uintptr_t GetFSBase();
    uintptr_t GetGSBase();
    uintptr_t GetKernelGSBase();

    void      SetGSBase(uintptr_t address);
    void      SetKernelGSBase(uintptr_t address);

    std::vector<CPU>& GetCPUs();
    u64               GetOnlineCPUsCount();
    u64               GetBspIpatd();
    CPU&              GetBsp();

    u64               GetCurrentID();
    void              Reschedule(usize ms);

    void              EnablePAT();
}; // namespace CPU
