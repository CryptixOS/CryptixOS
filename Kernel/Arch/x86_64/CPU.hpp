/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Arch/x86_64/CPUContext.hpp"
#include "Arch/x86_64/GDT.hpp"
#include "Arch/x86_64/IDT.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"

#include <vector>

namespace CPU
{
    constexpr usize KERNEL_STACK_SIZE = 64_kib;
    constexpr usize USER_STACK_SIZE   = 2_mib;

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

        constexpr usize IA32_EFER_SYSCALL_ENABLE = Bit(0);
    }; // namespace MSR

    struct CPU
    {
        usize            lapicID;
        usize            id;
        TaskStateSegment tss;
    };

    void InitializeBSP();

    bool ID(u64 leaf, u64 subleaf, u64& rax, u64& rbx, u64& rcx, u64& rdx);

    bool GetInterruptFlag();
    void SetInterruptFlag();
    void Halt();

    void WriteMSR(u32 msr, u64 value);
    u64  ReadMSR(u32 msr);

    void SetGSBase(uintptr_t address);
    void SetKernelGSBase(uintptr_t address);

    void EnablePAT();
}; // namespace CPU
