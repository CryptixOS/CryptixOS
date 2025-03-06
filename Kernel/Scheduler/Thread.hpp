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
#include <Scheduler/Event.hpp>

#include <deque>
#include <errno.h>
#include <vector>

enum class ThreadState
{
    eReady,
    eIdle,
    eKilled,
    eRunning,
    eDequeued,
    eEnqueued,
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
    inline ErrorCode& GetError() { return m_ErrorCode; }

    inline Process*   GetParent() const { return m_Parent; }
    constexpr bool    IsUser() const { return m_IsUser; }

    inline bool       IsEnqueued() const { return m_IsEnqueued; }
    constexpr bool    IsDead() const
    {
        return m_State == ThreadState::eExited
            || m_State == ThreadState::eKilled;
    }
    constexpr bool IsBlocked() const
    {
        return m_State == ThreadState::eBlocked;
    }
    constexpr bool ReadyForCleanup() { return IsDead(); }

    Thread*        Fork(Process* parent);

    // FIXME(v1tr10l7): implement this once we have signals
    inline bool    WasInterrupted() const { return false; }

#ifdef CTOS_TARGET_X86_64
    inline uintptr_t GetFsBase() const { return m_FsBase; }
    inline uintptr_t GetGsBase() const { return m_GsBase; }

    inline void      SetFsBase(uintptr_t fs) { m_FsBase = fs; }
    inline void      SetGsBase(uintptr_t gs) { m_GsBase = gs; }
#endif

    inline Event&              GetEvent() { return m_Event; }
    inline std::deque<Event*>& GetEvents() { return m_Events; }
    inline usize               GetWhich() const { return m_Which; }

    inline void                SetWhich(usize which) { m_Which = which; }

    // DON'T MOVE
    //////////////////////
    usize                      runningOn;
    Thread*                    self;
    uintptr_t                  stack;

    uintptr_t                  kernelStack;
    uintptr_t                  pageFaultStack;

    usize                      fpuStoragePageCount;
    uintptr_t                  fpuStorage;
    //////////////////////

    Spinlock                   m_Lock;
    tid_t                      m_Tid;
    ThreadState                m_State = ThreadState::eIdle;
    errno_t                    m_ErrorCode;
    Process*                   m_Parent;
    uintptr_t                  m_StackVirt;

    CPUContext                 ctx;
    CPUContext                 SavedContext;
    Spinlock                   YieldAwaitLock;

    std::vector<std::pair<uintptr_t, usize>> m_Stacks;
    bool                                     m_IsUser = false;

#if CTOS_ARCH == CTOS_ARCH_X86_64
    uintptr_t m_GsBase;
    uintptr_t m_FsBase;
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    uintptr_t m_El0Base;
#endif

    bool               m_IsEnqueued = false;
    Event              m_Event;
    std::deque<Event*> m_Events;
    usize              m_Which = 0;

    friend class Process;
    friend class Scheduler;
};
