/*
 * Created by v1tr10l7 on 25.05.2024.
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
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
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

export module Cryptix:Arch.x86_64.CPU;

export import :Debug.Assertions;
export import :Library.Logger;
export import :Library.Stacktrace;
export import :Arch.x86_64.ExecutionContext;
export import :Arch.x86_64.CPUID;
export import :Arch.x86_64.CPUIntrinsics;
export import :Arch.x86_64.GDT;
export import :Arch.x86_64.IDT;
export import :Memory.PMM;
export import :Memory.VMM;

export class ClockSource;
export struct Thread;
namespace CPU
{
    export using FPUSaveFunc    = void (*)(upointer ctx);
    export using FPURestoreFunc = void (*)(upointer ctx);

    export namespace PAT
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

    export namespace MSR
    {
        constexpr usize IA32_APIC_BASE           = 0x1b;
        constexpr usize IA32_APIC_BASE_BSP       = Bit(8);
        constexpr usize APIC_ENABLE_X2APIC       = Bit(10);
        constexpr usize IA32_APIC_GLOBAL_ENABLE  = Bit(11);

        constexpr usize IA32_PAT                 = 0x277;
        constexpr usize IA32_PAT_RESET           = 0x0007040600070406;

        constexpr usize KVM_SYSTEM_TIME          = 0x4b564d01;

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

    export namespace XCR0
    {
        constexpr usize X87       = Bit(0);
        constexpr usize SSE       = Bit(1);
        constexpr usize AVX       = Bit(2);
        constexpr usize OPMASK    = Bit(5);
        constexpr usize ZMM_HI256 = Bit(6);
        constexpr usize ZMM_HI16  = Bit(7);
    }; // namespace XCR0
    export namespace CR0
    {
        // Coprocessor monitoring
        constexpr usize MP = Bit(1);
        // Coprocessor emulation
        constexpr usize EM = Bit(2);
    }; // namespace CR0
    export namespace CR4
    {
        constexpr usize OSFXSR     = Bit(9);
        constexpr usize OSXMMEXCPT = Bit(10);
        constexpr usize UMIP       = Bit(11);
        constexpr usize OSXSAVE    = Bit(18);
        constexpr usize SMEP       = Bit(20);
        constexpr usize SMAP       = Bit(21);
    }; // namespace CR4

    export struct CPU
    {
        usize            ID    = 0;
        void*            Empty = nullptr;

        upointer         ThreadStack;
        upointer         KernelStack;

        u64              LapicID  = 0;
        bool             IsOnline = false;
        TaskStateSegment TSS{};

        usize            FpuStorageSize = 512;
        upointer         FpuStorage     = 0;

        FPUSaveFunc      FpuSave        = nullptr;
        FPURestoreFunc   FpuRestore     = nullptr;

        Spinlock*        Lock;
        bool             DuringSyscall = false;
        usize            LastSyscallID = usize(-1);

        ErrorCode        Error;
        Thread*          Idle;
        Thread*          CurrentThread;

        using HookType = IntrusiveRefListHook<CPU, CPU*>;
        friend class IntrusiveRefList<CPU, HookType>;
        friend struct IntrusiveRefListHook<CPU, CPU*>;

        using List = IntrusiveRefList<CPU, HookType>;
        HookType Hook;
    };

    export KERNEL_INIT_CODE
    void InitializeBSP();
    export KERNEL_INIT_CODE
    void StartAPs();

    export struct ID final
    {
        ID() = default;
        explicit ID(u64 leaf, u64 subleaf = 0);

        bool operator()(u64 leaf, u64 subleaf = 0);

        u64  rax, rbx, rcx, rdx;
    };

    export u32          KvmBase();
    export void         HaltAll();
    export void         WakeUp(usize id, bool everyone);

    export Pointer      GetFSBase();
    export Pointer      GetGSBase();
    export Pointer      GetKernelGSBase();

    export void         SetFSBase(Pointer address);
    export void         SetGSBase(Pointer address);
    export void         SetKernelGSBase(Pointer address);

    export u64          GetBspId();
    export CPU&         GetBsp();
    export CPU&         GetCPU(usize id);

    export CPU::List&   GetCPUs();
    export u64          GetOnlineCPUsCount();
    export u64          GetCurrentID();

    export ClockSource* HighResolutionClock();

    export void         Reschedule(Timestep interval);

    export bool         EnableSSE();
    export void         EnablePAT();
    export void         EnableSMEP();
    export void         EnableSMAP();
    export void         EnableUMIP();
}; // namespace CPU
