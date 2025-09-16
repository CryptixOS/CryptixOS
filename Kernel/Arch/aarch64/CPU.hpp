/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/aarch64/CPUIntrinsics.hpp>
#include <Library/Locking/Spinlock.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>

struct Thread;
namespace CPU
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
