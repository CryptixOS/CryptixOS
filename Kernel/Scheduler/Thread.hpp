/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/Posix/signal.h>
#include <API/UnixTypes.hpp>

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include <Arch/x86_64/CPUContext.hpp>
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    #include <Arch/aarch64/CPUContext.hpp>
#endif

#include <Library/ExecutableProgram.hpp>
#include <Memory/Region.hpp>

#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Scheduler/Event.hpp>

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
    Thread(Process* parent, Pointer pc, Pointer arg, i64 runOn = -1);
    Thread(Process* parent, Vector<StringView>& arg, Vector<StringView>& envp,
           ExecutableProgram& program, i64 runOn = -1);
    Thread(Process* parent, Pointer pc, bool user = true);
    ~Thread();

    static Thread*     Current();

    void               SetRunningOn(isize runningOn);

    Pointer            GetStack() const;
    void               SetStack(Pointer stack);

    Pointer            PageFaultStack() const;
    Pointer            KernelStack() const;

    void               SetPageFaultStack(Pointer pfstack);
    void               SetKernelStack(Pointer kstack);

    Pointer            FpuStorage() const;
    usize              FpuStoragePageCount() const;

    void               SetFpuStorage(Pointer fpuStorage, usize pageCount);

    inline tid_t       Tid() const { return m_Tid; }
    inline ThreadState State() const { return m_State; }
    inline void        SetState(ThreadState state)
    {
        ScopedLock guard(m_Lock);
        m_State = state;
    }
    inline ErrorCode& ErrorCode() { return m_ErrorCode; }

    inline Process*   Parent() const { return m_Parent; }
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
    constexpr bool  ReadyForCleanup() { return IsDead(); }

    Thread*         Fork(Process* parent);

    inline sigset_t SignalMask() const { return m_SignalMask; }
    inline void     SetSignalMask(sigset_t mask) { m_SignalMask = mask; }

    inline bool     ShouldIgnoreSignal(u8 signal) const
    {
        return m_SignalMask & signal;
    }

    // FIXME(v1tr10l7): implement this once we have signals
    inline bool WasInterrupted() const { return false; }
    void        SendSignal(u8 signal);
    bool        DispatchAnyPendingSignal();
    bool        DispatchSignal(u8 signal);

#ifdef CTOS_TARGET_X86_64
    inline Pointer FsBase() const { return m_FsBase; }
    inline Pointer GsBase() const { return m_GsBase; }

    inline void    SetFsBase(Pointer fs) { m_FsBase = fs; }
    inline void    SetGsBase(Pointer gs) { m_GsBase = gs; }
#endif

    inline Event&                Event() { return m_Event; }
    inline Deque<struct Event*>& Events() { return m_Events; }
    inline usize                 Which() const { return m_Which; }

    inline void                  SetWhich(usize which) { m_Which = which; }

    using List = IntrusiveList<Thread>;

  private:
    ///// DON'T MOVE /////
    isize       m_RunningOn = -1;
    Thread*     m_Self      = this;
    Pointer     m_Stack;

    Pointer     m_KernelStack;
    Pointer     m_PageFaultStack;

    usize       m_FpuStoragePageCount;
    Pointer     m_FpuStorage;
    ///// ^^^^^^^^^^ /////

    Spinlock    m_Lock;
    tid_t       m_Tid;
    ThreadState m_State     = ThreadState::eIdle;
    errno_t     m_ErrorCode = no_error;
    Process*    m_Parent;
    Pointer     m_StackVirt;

  public:
    CPUContext Context;
    CPUContext SavedContext;
    Spinlock   YieldAwaitLock;

  private:
    Vector<Ref<Region>> m_Stacks;
    bool                m_IsUser = false;

#if CTOS_ARCH == CTOS_ARCH_X86_64
    Pointer m_GsBase;
    Pointer m_FsBase;
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    Pointer m_El0Base;
#endif

    bool                 m_IsEnqueued     = false;
    sigset_t             m_SignalMask     = 0;
    sigset_t             m_PendingSignals = 0;

    struct Event         m_Event;
    Deque<struct Event*> m_Events;
    usize                m_Which = 0;

    friend class IntrusiveList<Thread>;
    friend struct IntrusiveListHook<Thread>;
    friend struct ThreadQueue;

    IntrusiveListHook<Thread> Hook;

    friend class Process;
    friend class Scheduler;
};
