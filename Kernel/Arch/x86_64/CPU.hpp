/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Arch/CPU.hpp>

#include <Arch/x86_64/CPUContext.hpp>
#include <Arch/x86_64/CPUID.hpp>
#include <Arch/x86_64/Drivers/Timers/Lapic.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <errno.h>
#include <vector>

struct Thread;
namespace CPU
{
    using FPUSaveFunc    = void (*)(uintptr_t ctx);
    using FPURestoreFunc = void (*)(uintptr_t ctx);

    namespace PAT
    {
        enum Entries : u64
        {
            eUncacheableStrong = 0x00,
            eWriteCombining    = 0x01,
            eWriteThrough      = 0x04,
            eWriteProtected    = 0x05,
            eWriteBack         = 0x06,
            eUncacheable       = 0x07,
        };
    };

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

    namespace XCR0
    {
        constexpr usize X87       = Bit(0);
        constexpr usize SSE       = Bit(1);
        constexpr usize AVX       = Bit(2);
        constexpr usize OPMASK    = Bit(5);
        constexpr usize ZMM_HI256 = Bit(6);
        constexpr usize ZMM_HI16  = Bit(7);
    }; // namespace XCR0
    namespace CR0
    {
        // Coprocessor monitoring
        constexpr usize MP = Bit(1);
        // Coprocessor emulation
        constexpr usize EM = Bit(2);
    }; // namespace CR0
    namespace CR4
    {
        constexpr usize OSFXSR     = Bit(9);
        constexpr usize OSXMMEXCPT = Bit(10);
        constexpr usize UMIP       = Bit(11);
        constexpr usize OSXSAVE    = Bit(18);
        constexpr usize SMEP       = Bit(20);
        constexpr usize SMAP       = Bit(21);
    }; // namespace CR4

    struct CPU
    {
        usize            ID    = 0;
        void*            Empty = nullptr;

        uintptr_t        ThreadStack;
        uintptr_t        KernelStack;

        u64              LapicID  = 0;
        bool             IsOnline = false;
        TaskStateSegment TSS{};

        usize            FpuStorageSize = 512;
        uintptr_t        FpuStorage     = 0;

        FPUSaveFunc      FpuSave        = nullptr;
        FPURestoreFunc   FpuRestore     = nullptr;

        errno_t          Error;
        Thread*          Idle;
        Thread*          CurrentThread;
    };

    void InitializeBSP();
    void StartAPs();

    struct ID final
    {
        ID() = default;
        ID(u64 leaf, u64 subleaf = 0);

        bool operator()(u64 leaf, u64 subleaf = 0);

        u64  rax, rbx, rcx, rdx;
    };

    void              Halt();
    void              HaltAll();
    void              WakeUp(usize id, bool everyone);

    void              WriteXCR(u64 reg, u64 value);
    u64               ReadCR0();
    u64               ReadCR2();
    void              WriteCR0(u64 value);
    u64               ReadCR4();
    void              WriteCR4(u64 value);

    u64               ReadMSR(u32 msr);
    void              WriteMSR(u32 msr, u64 value);

    uintptr_t         GetFSBase();
    uintptr_t         GetGSBase();
    uintptr_t         GetKernelGSBase();

    void              SetFSBase(uintptr_t address);
    void              SetGSBase(uintptr_t address);
    void              SetKernelGSBase(uintptr_t address);

    std::vector<CPU>& GetCPUs();
    u64               GetOnlineCPUsCount();
    u64               GetBspId();
    CPU&              GetBsp();

    u64               GetCurrentID();
    void              Reschedule(TimeStep interval);

    bool              EnableSSE();
    void              EnablePAT();
    void              EnableSMEP();
    void              EnableSMAP();
    void              EnableUMIP();
}; // namespace CPU
