/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Arch/aarch64/CPUIntrinsics.hpp>
#include <Compiler.hpp>
#include <Prism/Core/Types.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>

export module Cryptix:Arch.aarch64.CPU;

export import :Arch.aarch64.CPUIntrinsics;
export import :Library.Locking.Spinlock;

export struct Thread;
export namespace CPU
{
    struct CPU
    {
        usize     ID;
        void*     Empty;

        upointer  ThreadStack;
        upointer  KernelStack;

        usize     HardwareID;
        bool      IsOnline       = false;

        usize     FpuStorageSize = 512;
        upointer  FpuStorage     = 0;

        Spinlock* Lock;
        bool      DuringSyscall = false;
        usize     LastSyscallID = usize(-1);

        ErrorCode Error;
        Thread*   Idle;
        Thread*   CurrentThread;

        using HookType = IntrusiveRefListHook<CPU, CPU*>;
        friend class IntrusiveRefList<CPU, HookType>;
        friend struct IntrusiveRefListHook<CPU, CPU*>;

        using List = IntrusiveRefList<CPU, HookType>;
        HookType Hook;
    };

    u64        GetBspId();
    CPU&       GetBsp();
    CPU&       GetCPU(usize id);

    CPU::List& GetCPUs();
    u64        GetOnlineCPUsCount();
    u64        GetCurrentID();
}; // namespace CPU
