/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Scheduler/Scheduler.hpp>

#include <Arch/CPU.hpp>
#include <Arch/InterruptHandler.hpp>
#include <Arch/InterruptManager.hpp>
#include <Arch/x86_64/Drivers/IoApic.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Memory/PMM.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>
#include <Time/Time.hpp>

#include <VFS/MountPoint.hpp>
#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/VFS.hpp>

namespace
{
    Spinlock s_Lock{};
    bool     s_SchedulerEnabled = false;

    struct CPULocalData
    {
        Atomic<bool> PreemptionEnabled;
    };
    CPULocalData*                 s_CPULocalData;

    Process*                      s_KernelProcess = nullptr;
    Spinlock                      s_ProcessListLock;
    UnorderedMap<pid_t, Process*> s_Processes;
    ProcFs*                       s_ProcFs = nullptr;
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
    Time::GetSchedulerTimer()->SetCallback<Tick>();

    LogInfo("Scheduler: Kernel process created");
    LogInfo("Scheduler: Initialized");
}
KERNEL_INIT_CODE
void Scheduler::InitializeProcFs()
{
    VFS::CreateNode(nullptr, "/proc", 0755 | S_IFDIR);
    Assert(VFS::Mount(nullptr, "", "/proc", "procfs"));

    s_ProcFs = reinterpret_cast<ProcFs*>(MountPoint::Head()->Filesystem());
    s_ProcFs->AddProcess(s_KernelProcess);
}
void Scheduler::PrepareAP(bool start)
{
    Process* process    = Process::CreateIdleProcess();

    auto     idleThread = process->CreateThread(
        reinterpret_cast<uintptr_t>(Arch::Halt), false, CPU::Current()->ID);
    idleThread->SetState(ThreadState::eReady);

    CPU::Current()->Idle = idleThread;

    if (start)
    {
        CPU::SetInterruptFlag(true);

        CPU::WakeUp(0, true);
        for (;;) Arch::Halt();
    }
};

bool Scheduler::IsPreemptionEnabled()
{
    if (!s_SchedulerEnabled) return false;

    u64 cpuID = CPU::GetCurrentID();
    return s_CPULocalData[cpuID].PreemptionEnabled.Load(
        MemoryOrder::eAtomicRelaxed);
}

void Scheduler::EnablePreemption()
{
    if (!s_SchedulerEnabled) return;

    u64 cpuID = CPU::GetCurrentID();
    s_CPULocalData[cpuID].PreemptionEnabled.Store(true,
                                                  MemoryOrder::eAtomicRelease);
}
void Scheduler::DisablePreemption()
{
    if (!s_SchedulerEnabled) return;

    u64 cpuID = CPU::GetCurrentID();
    s_CPULocalData[cpuID].PreemptionEnabled.Store(false,
                                                  MemoryOrder::eAtomicAcquire);
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

#ifdef CTOS_TARGET_X86_64
    Lapic::Instance()->SendIpi(Time::GetSchedulerTimer()->InterruptVector(),
                               CPU::Current()->LapicID);
#endif
}

void Scheduler::Yield(bool saveCtx)
{
    CPU::SetInterruptFlag(false);
#ifdef CTOS_TARGET_X86_64
    Lapic::Instance()->Stop();
#endif

    Thread* currentThread = Thread::Current();
    if (saveCtx) currentThread->YieldAwaitLock.Acquire();
    else
    {
#ifdef CTOS_TARGET_X86_64
        CPU::SetGSBase(reinterpret_cast<uintptr_t>(CPU::Current()->Idle));
        CPU::SetKernelGSBase(reinterpret_cast<uintptr_t>(CPU::Current()->Idle));
#endif
    }

#ifdef CTOS_TARGET_X86_64
    Lapic::Instance()->SendIpi(Time::GetSchedulerTimer()->InterruptVector(),
                               CPU::Current()->LapicID);
#endif

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

Process* Scheduler::CreateProcess(Process* parent, StringView name,
                                  const Credentials& creds)
{
    auto       proc = new Process(parent, name, creds);

    ScopedLock guard(s_ProcessListLock);
    s_Processes[proc->Pid()] = proc;
    s_ProcFs->AddProcess(proc);

    return proc;
}
void Scheduler::RemoveProcess(pid_t pid)
{
    ScopedLock guard(s_ProcessListLock);
    s_Processes.Erase(pid);
    s_ProcFs->RemoveProcess(pid);
}

bool Scheduler::ValidatePid(pid_t pid)
{
    ScopedLock guard(s_ProcessListLock);
    return s_Processes.Contains(pid);
}
Process* Scheduler::GetProcess(pid_t pid)
{
    ScopedLock guard(s_ProcessListLock);
    auto       it = s_Processes.Find(pid);

    return it != s_Processes.end() ? s_Processes[pid] : nullptr;
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

UnorderedMap<pid_t, Process*>& Scheduler::GetProcessMap()
{
    return s_Processes;
}

Thread* Scheduler::GetNextThread(usize cpuID)
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
        if (newThread->IsDead())
        {
            delete newThread;
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

    newThread->DispatchAnyPendingSignal();
    return newThread;
}
void Scheduler::SwitchContext(Thread* newThread, CPUContext* oldContext)
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
        && currentThread != CPU::Current()->Idle)
        delete currentThread;
}

void Scheduler::Tick(CPUContext* ctx)
{
    Thread* newThread = nullptr;
    if (!IsPreemptionEnabled()) goto reschedule;

    newThread = PickReadyThread();
    Assert(newThread);
    SwitchContext(newThread, ctx);

    if (newThread != CPU::Current()->Idle)
        newThread->SetState(ThreadState::eRunning);

reschedule:
    CPU::Reschedule(newThread->Parent()->m_Quantum * 1_ms);
}
