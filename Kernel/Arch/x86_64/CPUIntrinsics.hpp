/*
 * Created by v1tr10l7 on 31.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/x86_64/Types.hpp>
#include <Compiler.hpp>

namespace CPU
{
#define Asm(...) __asm__ volatile(__VA_ARGS__)

    CTOS_ALWAYS_INLINE static void Halt() { Asm("hlt"); }

    CTOS_ALWAYS_INLINE static void cpuid(u64 leaf, u64 subleaf, u64& rax,
                                            u64& rbx, u64& rcx, u64& rdx)
    {
        Asm("cpuid" : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx) : "a"(leaf),
            "c"(subleaf));
    }
    CTOS_ALWAYS_INLINE static u64 Flags()
    {
        u64 rflags = 0;
        Asm("pushf\n"
            "pop %0" : "=m"(rflags));

        return rflags;
    }
    CTOS_ALWAYS_INLINE static bool InterruptsEnabled()
    {
        return Flags() & Bit(9);
    }
    CTOS_ALWAYS_INLINE static u64 ReadCR0()
    {
        u64 cr0;
        Asm("mov %%cr0, %0" : "=r"(cr0)::"memory");

        return cr0;
    }
    CTOS_ALWAYS_INLINE static u64 ReadCR1()
    {
        u64 cr1;
        Asm("mov %%cr1, %0" : "=r"(cr1)::"memory");

        return cr1;
    }
    CTOS_ALWAYS_INLINE static u64 ReadCR2()
    {
        u64 cr2;
        Asm("mov %%cr2, %0" : "=r"(cr2)::"memory");

        return cr2;
    }
    CTOS_ALWAYS_INLINE static u64 ReadCR3()
    {
        u64 cr3;
        Asm("mov %%cr3, %0" : "=r"(cr3)::"memory");

        return cr3;
    }
    CTOS_ALWAYS_INLINE static u64 ReadCR4()
    {
        u64 cr4;
        Asm("mov %%cr4, %0" : "=r"(cr4)::"memory");

        return cr4;
    }
    CTOS_ALWAYS_INLINE static u64 ReadMSR(u32 msr)
    {
        u64 low  = 0;
        u64 high = 0;

        Asm("rdmsr" : "=a"(low), "=d"(high) : "c"(msr) : "memory");
        return (high << 32) | low;
    }

    CTOS_ALWAYS_INLINE static u64 ReadTsc()
    {
        u64 low  = 0;
        u64 high = 0;

        Asm("lfence; rdtsc" : "=a"(low), "=d"(high));
        return static_cast<u64>(low) | (static_cast<u64>(high) << 32);
    }

    CTOS_ALWAYS_INLINE static void SetFlags(u64 flags)
    {
        Asm("push %0\n"
            "popf" : : "m"(flags));
    }
    CTOS_ALWAYS_INLINE static void EnableInterrupts() { Asm("sti"); }
    CTOS_ALWAYS_INLINE static void DisableInterrupts() { Asm("cli"); }
    CTOS_ALWAYS_INLINE static void EnableUserAccess() { Asm("stac"); }
    CTOS_ALWAYS_INLINE static void DisableUserAccess() { Asm("clac"); }

    CTOS_ALWAYS_INLINE static void WriteCR0(u64 value)
    {
        Asm("mov %0, %%cr0" ::"r"(value) : "memory");
    }
    CTOS_ALWAYS_INLINE static void WriteCR1(u64 value)
    {
        Asm("mov %0, %%cr1" ::"r"(value) : "memory");
    }
    CTOS_ALWAYS_INLINE static void WriteCR2(u64 value)
    {
        Asm("mov %0, %%cr2" ::"r"(value) : "memory");
    }
    CTOS_ALWAYS_INLINE static void WriteCR3(u64 value)
    {
        Asm("mov %0, %%cr3" ::"r"(value) : "memory");
    }
    CTOS_ALWAYS_INLINE static void WriteCR4(u64 value)
    {
        Asm("mov %0, %%cr4" ::"r"(value) : "memory");
    }
    CTOS_ALWAYS_INLINE static void WriteXCR(u64 reg, u64 value)
    {
        u32 low  = value;
        u32 high = value >> 32;
        Asm("xsetbv" ::"a"(low), "d"(high), "c"(reg) : "memory");
    }
    CTOS_ALWAYS_INLINE static void WriteMSR(u32 msr, u64 value)
    {
        const u64 high = value >> 32;
        const u64 low  = value;
        Asm("wrmsr" : : "a"(low), "d"(high), "c"(msr) : "memory");
    }
}; // namespace CPU
