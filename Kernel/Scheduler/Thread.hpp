/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include <Arch/x86_64/CPUContext.hpp>
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    #include <Arch/aarch64/CPUContext.hpp>
#endif

#include <cerrno>
#include <vector>

#include <Utility/ELF.hpp>

using tid_t = i64;

enum class ThreadState
{
    eReady,
    eIdle,
    eKilled,
    eRunning,
    eDequeued,
    eBlocked,
    eExited,
};

struct Process;
struct Thread
{
    Thread() = default;
    Thread(Process* parent, uintptr_t pc, uintptr_t arg, i64 runOn = -1);
    Thread(Process* parent, uintptr_t pc, char** arg, char** envp,
           ELF::Image& program, i64 runOn = -1);
    Thread(Process* parent, uintptr_t pc, bool user = true);
    ~Thread();

    usize                                    runningOn;
    Thread*                                  self;
    uintptr_t                                stack;

    uintptr_t                                kernelStack;
    uintptr_t                                pageFaultStack;

    usize                                    fpuStoragePageCount;
    uintptr_t                                fpuStorage;

    tid_t                                    tid;
    errno_t                                  error;
    Process*                                 parent;

    CPUContext                               ctx;

    std::vector<std::pair<uintptr_t, usize>> stacks;
    bool                                     user = false;

#if CTOS_ARCH == CTOS_ARCH_X86_64
    uintptr_t gsBase;
    uintptr_t fsBase;
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    uintptr_t el0Base;
#endif

    bool        enqueued = false;
    ThreadState state    = ThreadState::eIdle;
};
