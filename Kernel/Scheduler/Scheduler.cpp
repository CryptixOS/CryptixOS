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
#include <Arch/aarch64/CPUIntrinsics.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Core/Singleton.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Memory/RefCounted.hpp>
#endif
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/UnixTypes.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Config.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.Scheduler.Scheduler;

import :Arch.CPU;
#if defined(CTOS_TARGET_X86_64)
import :Arch.x86_64.Drivers.Time.Lapic;
#endif
import :Library.Locking.Spinlock;
import :Memory.PMM;
import :Scheduler.Process;
import :Scheduler.Scheduler;
import :Scheduler.Thread;
import :Time.Time;
import :VFS.MountPoint;
import :VFS.VFS;

namespace
{
    Spinlock s_Lock{};
    bool     s_SchedulerEnabled = false;

    struct CPULocalData
    {
        Atomic<bool> PreemptionEnabled;
    };
    CPULocalData* s_CPULocalData;

    Process*      s_KernelProcess = nullptr;
    Spinlock      s_ProcessListLock;
    Process::List s_ProcessList;
} // namespace

struct ThreadQueue
{
    mutable Spinlock Lock;
    Thread::List     Queue;

    constexpr usize  Size() const { return Queue.Size(); }
    constexpr bool   IsEmpty() const { return Queue.Empty(); }

    auto             begin() { return Queue.begin(); }
    auto             end() { return Queue.end(); }

    inline Thread*   Front()
    {
        ScopedLock guard(Lock);
        return Queue.Head();
    }
    inline Thread* Back()
    {
        ScopedLock guard(Lock);
        return Queue.Tail();
    }

    inline void PushBack(Thread* t)
    {
        ScopedLock guard(Lock);
        Queue.PushBack(t);
    }
    inline void PushFront(Thread* t)
    {
        ScopedLock guard(Lock);
        Queue.PushFront(t);
    }

    inline void PopBack()
    {
        if (IsEmpty()) return;

        ScopedLock guard(Lock);
        Queue.PopBack();
    }
    inline void PopFront()
    {
        if (IsEmpty()) return;

        ScopedLock guard(Lock);
        Queue.PopFront();
    }

    inline Thread* PopFrontElement()
    {
        if (IsEmpty()) return nullptr;

        ScopedLock guard(Lock);
        return Queue.PopFrontElement();
    }
    inline Thread* PopBackElement()
    {
        if (IsEmpty()) return nullptr;
        ScopedLock guard(Lock);

        return Queue.PopBackElement();
    }

    inline void Erase(auto it)
    {
        ScopedLock guard(Lock);
        Queue.Erase(it);
    }
};

constexpr usize                               QUEUE_TYPE_EXECUTION = 0;
constexpr usize                               QUEUE_TYPE_WAIT      = 1;
constexpr usize                               QUEUE_TYPE_READY     = 2;
constexpr usize                               QUEUE_TYPE_BLOCKED   = 3;
constexpr usize                               QUEUE_TYPE_COUNT     = 4;

Vector<Array<ThreadQueue*, QUEUE_TYPE_COUNT>> s_Queues;

ThreadQueue&                                  ExecutionQueue(u64 cpuID)
{
    return *(s_Queues[cpuID][QUEUE_TYPE_EXECUTION]);
}
ThreadQueue& WaitQueue(u64 cpuID)
{
    return *(s_Queues[cpuID][QUEUE_TYPE_WAIT]);
}
ThreadQueue& ReadyQueue(u64 cpuID)
{
    return *(s_Queues[cpuID][QUEUE_TYPE_READY]);
}
ThreadQueue& BlockedQueue(u64 cpuID)
{
    return *(s_Queues[cpuID][QUEUE_TYPE_BLOCKED]);
}

KERNEL_INIT_CODE
void Scheduler::Initialize()
{
    u64 cpuCount    = CPU::GetOnlineCPUsCount();
    s_CPULocalData  = new CPULocalData[cpuCount];
    s_KernelProcess = Process::CreateKernelProcess();

    s_Queues.Resize(cpuCount);
    for (usize cpuID = 0; cpuID < cpuCount; cpuID++)
    {
        for (usize queueIndex = 0; queueIndex < QUEUE_TYPE_COUNT; queueIndex++)
            s_Queues[cpuID][queueIndex] = new ThreadQueue;
    }

    for (usize i = 0; i < cpuCount; i++)
        s_CPULocalData[i].PreemptionEnabled.Store(true);
    s_SchedulerEnabled = true;
    Time::SchedulerTimer()->SetCallback<Tick>();

    LogInfo("Scheduler: Kernel process created");
    LogInfo("Scheduler: Initialized");
}
void Scheduler::PrepareAP(bool start)
{
    Process* process = Process::CreateIdleProcess();

    auto     idleThread
        = process->CreateThread(Arch::Halt, false, CPU::Current()->ID);
    idleThread->SetState(ThreadState::eReady);

    CPU::Current()->Idle = idleThread.Raw();

    if (start)
    {
        CPU::SetInterruptFlag(true);

        Time::SchedulerTimer()->Start(TimerMode::eOneShot, 1'000_ms);
        // CPU::WakeUp(0, true);
        for (;;) Arch::Halt();
    }
};

bool Scheduler::IsPreemptionEnabled()
{
    if (!s_SchedulerEnabled) return false;

    u64 cpuID = CPU::GetCurrentID();
    return s_CPULocalData[cpuID].PreemptionEnabled.Load(MemoryOrder::eRelaxed);
}

void Scheduler::EnablePreemption()
{
    if (!s_SchedulerEnabled) return;

    u64 cpuID = CPU::GetCurrentID();
    s_CPULocalData[cpuID].PreemptionEnabled.Store(true, MemoryOrder::eRelease);
}
void Scheduler::DisablePreemption()
{
    if (!s_SchedulerEnabled) return;

    u64 cpuID = CPU::GetCurrentID();
    s_CPULocalData[cpuID].PreemptionEnabled.Store(false, MemoryOrder::eAcquire);
}

void Scheduler::Block(Thread* thread)
{
    if (thread->State() == ThreadState::eBlocked) return;
    thread->SetState(ThreadState::eBlocked);

    if (thread == Thread::Current()) Yield(true);
}
void Scheduler::Unblock(Thread* thread)
{
    if (thread->State() != ThreadState::eBlocked) return;

    thread->SetState(ThreadState::eReady);
    if (!thread->m_IsEnqueued)
    {
        if (thread->Hook.IsLinked()) thread->Hook.Unlink(thread);
        EnqueueThread(thread);
    }

    Time::SchedulerTimer()->Start(TimerMode::eOneShot,
                                  thread->Parent()->Quantum());
}

void Scheduler::Yield(bool saveCtx)
{
    CPU::SetInterruptFlag(false);
    Time::SchedulerTimer()->Stop();

    Thread* currentThread = Thread::Current();
    if (saveCtx) currentThread->YieldAwaitLock.Acquire();
    else
    {
#ifdef CTOS_TARGET_X86_64
        CPU::SetGSBase(
            reinterpret_cast<upointer>(&CPU::Current()->Idle->m_Tls));
        CPU::SetKernelGSBase(
            reinterpret_cast<upointer>(&CPU::Current()->Idle->m_Tls));
#endif
    }

    Time::SchedulerTimer()->Start(TimerMode::eOneShot, 0);
    CPU::SetInterruptFlag(true);

    if (saveCtx)
    {
        currentThread->YieldAwaitLock.Acquire();
        currentThread->YieldAwaitLock.Release();
    }
    else
        for (;;) Arch::Halt();
}

Process* Scheduler::GetKernelProcess() { return s_KernelProcess; }
Process* Scheduler::KernelProcess() { return s_KernelProcess; }

Process* Scheduler::CreateProcess(Process* parent, StringView name,
                                  const Credentials& creds)
{
    auto       proc = new Process(parent, name, creds);

    ScopedLock guard(s_ProcessListLock);
    s_ProcessList.PushBack(proc);

    return proc;
}
void Scheduler::RemoveProcess(ProcessID pid)
{
    ScopedLock guard(s_ProcessListLock);

    s_ProcessListLock.Release();
    auto found = GetProcess(pid);
    s_ProcessListLock.Acquire();
    if (found) found->Hook.Unlink(found);
}

bool Scheduler::ValidatePid(ProcessID pid)
{
    ScopedLock guard(s_ProcessListLock);

    for (auto& process : s_ProcessList)
        if (process.ID() == pid) return true;
    return false;
}
Process* Scheduler::GetProcess(ProcessID pid)
{
    ScopedLock guard(s_ProcessListLock);
    for (auto& process : s_ProcessList)
        if (process.ID() == pid) return &process;

    return nullptr;
}

void Scheduler::EnqueueThread(Thread* thread)
{
    Assert(!thread->Hook.IsLinked());
    if (thread->m_IsEnqueued) return;

    ScopedLock guard(s_Lock);
    thread->m_IsEnqueued = true;
    thread->SetState(ThreadState::eReady);

    ExecutionQueue(CPU::GetCurrentID()).PushBack(thread);
}

void Scheduler::EnqueueNotReady(Thread* thread)
{
    Assert(!thread->Hook.IsLinked());
    ScopedLock guard(s_Lock);

    thread->m_IsEnqueued = true;
    if (thread->State() == ThreadState::eRunning)
        thread->SetState(ThreadState::eReady);

    ExecutionQueue(CPU::GetCurrentID()).PushBack(thread);
}
void Scheduler::DequeueThread(Thread* thread)
{
    Assert(thread->Hook.IsLinked());

    if (!thread->IsEnqueued()) return;
    ScopedLock guard(s_Lock);

    thread->m_IsEnqueued = false;
    thread->SetState(ThreadState::eDequeued);
    thread->Hook.Unlink(thread);
}

Process::List& Scheduler::ProcessList() { return s_ProcessList; }

Thread*        Scheduler::GetNextThread(usize cpuID)
{
    if (ExecutionQueue(CPU::GetCurrentID()).IsEmpty()) return nullptr;

    auto thread = ExecutionQueue(CPU::GetCurrentID()).PopFrontElement();
    if (!thread) return nullptr;

    thread->m_IsEnqueued = false;

    Assert(!thread->Hook.IsLinked());
    return thread;
}
Thread* Scheduler::PickReadyThread()
{
    auto newThread = GetNextThread(CPU::Current()->ID);

    for (; newThread && newThread->State() != ThreadState::eReady;)
    {
        if (newThread->IsDead() && newThread->Parent()->IsDead())
        {
            delete newThread->Parent();
            continue;
        }
        else if (newThread->IsBlocked())
        {
            BlockedQueue(CPU::GetCurrentID()).PushBack(newThread);
            newThread = GetNextThread(CPU::Current()->ID);
            continue;
        }

        EnqueueNotReady(newThread);
        newThread = GetNextThread(CPU::Current()->ID);
    }

    Thread* currentThread = Thread::Current();
    if (!newThread) return currentThread ? currentThread : CPU::Current()->Idle;

    return newThread;
}
void Scheduler::SwitchContext(Thread* newThread, ExecutionContext* oldContext)
{
    auto currentThread = CPU::GetCurrentThread();
    if (currentThread) currentThread->YieldAwaitLock.Release();

    if (currentThread && !currentThread->IsDead())
    {
        if (currentThread == newThread)
            return currentThread->YieldAwaitLock.Acquire();
        if (currentThread != CPU::Current()->Idle)
            EnqueueNotReady(currentThread);

        CPU::SaveThread(currentThread, oldContext);
    }

    CPU::LoadThread(newThread, oldContext);
    if (currentThread && currentThread->IsDead()
        && currentThread->Parent()->IsDead()
        && currentThread != CPU::Current()->Idle)
        delete currentThread->Parent();
}

void Scheduler::Tick(ExecutionContext* ctx)
{
    Thread* newThread = nullptr;
    if (!IsPreemptionEnabled()) goto reschedule;

    newThread = PickReadyThread();
    Assert(newThread);
    SwitchContext(newThread, ctx);
    if (newThread->DispatchAnyPendingSignal()) *ctx = newThread->Context;

    if (newThread != CPU::Current()->Idle)
        newThread->SetState(ThreadState::eRunning);

reschedule:
    Time::SchedulerTimer()->Start(TimerMode::eOneShot,
                                  newThread->Parent()->Quantum() * 1_ms);
}
