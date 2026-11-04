/*
 * Created by v1tr10l7 on 25.11.2024.
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
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Utility/Optional.hpp>

export module Cryptix:Scheduler.Thread;

export import :Debug.Assertions;
export import :Library.Logger;
export import :Library.Stacktrace;
export import :API.Signals;
#if (CTOS_ARCH == CTOS_ARCH_X86_64)
export import :Arch.x86_64.ExecutionContext;
#endif
#if !((CTOS_ARCH == CTOS_ARCH_X86_64)) && (CTOS_ARCH == CTOS_ARCH_AARCH64)
export import :Arch.aarch64.ExecutionContext;
#endif
export import :Library.ExecutableProgram;
export import :Memory.Region;
export import :Scheduler.Event;

export namespace CPU
{
    Thread* GetCurrentThread();
};

export enum class ThreadState
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

export struct Thread;
export struct ThreadTLS
{
    isize   RunningOn;
    Thread* Self;
    Pointer Stack;

    Pointer KernelStack;
    Pointer PageFaultStack;

    usize   FpuStoragePageCount;
    Pointer FpuStorage;
};

export class Process;
export struct Thread : public RefCounted
{
    Thread() = default;
    Thread(Process* parent, Pointer pc, Pointer arg, i64 runOn = -1,
           bool user = false);
    Thread(Process* parent, Vector<StringView>& arg, Vector<StringView>& envp,
           ExecutableProgram& program, i64 runOn = -1);
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

    inline ThreadID    ID() const { return m_ID; }
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
    constexpr bool         ReadyForCleanup() { return IsDead(); }

    ErrorOr<::Ref<Thread>> Clone(usize flags, Pointer stack, usize stackSize,
                                 Pointer tls);

    inline SignalSet       SignalMask() const { return m_SignalMask; }
    void                   SetSignalMask(SignalSet mask);

    inline bool            ShouldIgnoreSignal(u8 signal) const
    {
        return m_SignalMask.Contains(signal);
    }

    inline bool   ExecutingSyscall() const { return m_ExecutingSyscall.Load(); }
    inline bool   WasInterrupted() const { return false; }

    void          OnSyscallEnter();
    void          OnSyscallLeave();

    void          SendSignal(u8 signal);
    bool          DispatchAnyPendingSignal();
    bool          DispatchSignal(u8 signal);
    ErrorOr<void> SignalReturn();

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
    Spinlock       m_Lock;
    ThreadID       m_ID;
    ThreadState    m_State     = ThreadState::eIdle;
    ::ErrorCode    m_ErrorCode = no_error;
    Process*       m_Parent;
    Pointer        m_StackVirt;

    friend Thread* CPU::GetCurrentThread();

  public:
    ExecutionContext Context;
    ExecutionContext SavedContext;
    Spinlock         YieldAwaitLock;

  private:
    Vector<::Ref<Region>> m_Stacks;
    bool                  m_IsUser = false;

#if CTOS_ARCH == CTOS_ARCH_X86_64
    Pointer m_GsBase;
    Pointer m_FsBase;
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    Pointer m_El0Base;
#endif

    bool       m_IsEnqueued = false;

    Spinlock   m_SignalMaskLock;
    SignalSet  m_SignalMask       = {};
    SignalSet  m_PendingSignals   = {};
    AtomicBool m_ExecutingSyscall = false;

  public:
    ThreadTLS m_Tls;

  private:
    struct Event         m_Event;
    Deque<struct Event*> m_Events;
    usize                m_Which = 0;

    friend class IntrusiveList<Thread>;
    friend struct IntrusiveListHook<Thread>;
    friend struct ThreadQueue;

    IntrusiveListHook<Thread> Hook;

    friend class Process;
    friend class Scheduler;

    KeyValuePair<upointer, upointer> AllocateUserStack();
};
