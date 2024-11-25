/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include "Arch/x86_64/CPUContext.hpp"
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    #include "Arch/aarch64/CPUContext.hpp"
#endif

#include <cerrno>
#include <vector>

using tid_t = i64;

enum class ThreadState
{
    eIdle,
    eReady,
    eRunning,
    eKilled,
    eBlocked,
    eDequeued,
};

struct Process;
struct Thread
{
    Thread() = default;
    Thread(Process* parent, uintptr_t pc, bool user);
    ~Thread();

    usize     runningOn;
    Thread*   self;
    uintptr_t stack;

#if CTOS_ARCH == CTOS_ARCH_X86_64
    uintptr_t kstack;
    uintptr_t pfstack;
#endif

    tid_t      tid;
    errno_t    error;
    Process*   parent;

    CPUContext ctx;
    CPUContext savedCtx;

#if CTOS_ARCH == CTOS_ARCH_X86_64
    uintptr_t gsBase;
    uintptr_t fsBase;
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    uintptr_t el0Base;
#endif

    bool        user     = false;
    bool        enqueued = false;
    ThreadState state    = ThreadState::eIdle;
};
