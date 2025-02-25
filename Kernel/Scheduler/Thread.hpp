/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/UnixTypes.hpp>
#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include <Arch/x86_64/CPUContext.hpp>
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    #include <Arch/aarch64/CPUContext.hpp>
#endif

#include <Library/ELF.hpp>

#include <errno.h>
#include <vector>

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

class Process;
struct Thread
{
    Thread() = default;
    Thread(Process* parent, uintptr_t pc, uintptr_t arg, i64 runOn = -1);
    Thread(Process* parent, uintptr_t pc, std::vector<std::string_view>& arg,
           std::vector<std::string_view>& envp, ELF::Image& program,
           i64 runOn = -1);
    Thread(Process* parent, uintptr_t pc, bool user = true);
    ~Thread();

    static Thread*     GetCurrent();

    inline tid_t       GetTid() const { return m_Tid; }
    inline ThreadState GetState() const { return m_State; }
    inline void        SetState(ThreadState state)
    {
        ScopedLock guard(m_Lock);
        m_State = state;
    }

    Thread*     Fork(Process* parent);

    // FIXME(v1tr10l7): implement tthis once we have signals
    inline bool WasInterrupted() const { return false; }

    // DON'T MOVE
    //////////////////////
    usize       runningOn;
    Thread*     self;
    uintptr_t   stack;

    uintptr_t   kernelStack;
    uintptr_t   pageFaultStack;

    usize       fpuStoragePageCount;
    uintptr_t   fpuStorage;
    //////////////////////

    Spinlock    m_Lock;
    tid_t       m_Tid;
    ThreadState m_State = ThreadState::eIdle;
    errno_t     error;
    Process*    parent;
    uintptr_t   stackVirt;

    CPUContext  ctx;
    CPUContext  SavedContext;

    std::vector<std::pair<uintptr_t, usize>> stacks;
    bool                                     user = false;

#if CTOS_ARCH == CTOS_ARCH_X86_64
    uintptr_t gsBase;
    uintptr_t fsBase;
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    uintptr_t el0Base;
#endif

    bool enqueued = false;
};
