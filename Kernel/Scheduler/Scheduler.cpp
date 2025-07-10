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

u8 g_ScheduleVector = 48;
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

        t->Hook.Unlink(t);
        Queue.PushBack(t);
    }
    inline void PushFront(Thread* t)
    {
        ScopedLock guard(Lock);

        t->Hook.Unlink(t);
        Queue.PushFront(t);
    }

    inline void PopBack()
    {
        ScopedLock guard(Lock);
        Queue.PopBack();
    }
    inline void PopFront()
    {
        ScopedLock guard(Lock);
        Queue.PopFront();
    }

    inline Thread* PopFrontElement()
    {
        ScopedLock guard(Lock);
        return Queue.PopFrontElement();
    }
    inline Thread* PopBackElement()
    {
        ScopedLock guard(Lock);
        return Queue.PopBackElement();
    }

    inline void Erase(auto it)
    {
        ScopedLock guard(Lock);
        Queue.Erase(it);
    }
};

ThreadQueue s_ExecutionQueue;
ThreadQueue s_WaitQueue;
ThreadQueue s_ReadyQueue;
ThreadQueue s_BlockedQueue;

KERNEL_INIT_CODE
void Scheduler::Initialize()
{
    u64 cpuCount    = CPU::GetOnlineCPUsCount();
    s_CPULocalData  = new CPULocalData[cpuCount];
    s_KernelProcess = Process::CreateKernelProcess();

    Time::Initialize();

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
    if (!thread->m_IsEnqueued) EnqueueThread(thread);

    for (auto it = s_BlockedQueue.begin(); it != s_BlockedQueue.end(); it++)
        if (it.Current == thread)
        {
            s_BlockedQueue.Erase(it);
            break;
        }
#ifdef CTOS_TARGET_X86_64
    Lapic::Instance()->SendIpi(g_ScheduleVector, CPU::Current()->LapicID);
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
    Lapic::Instance()->SendIpi(g_ScheduleVector, CPU::Current()->LapicID);
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
    ScopedLock guard(s_Lock);

    if (thread->m_IsEnqueued) return;

    thread->m_IsEnqueued = true;
    thread->SetState(ThreadState::eReady);
    s_ExecutionQueue.PushBack(thread);
}

void Scheduler::EnqueueNotReady(Thread* thread)
{
    ScopedLock guard(s_Lock);

    thread->m_IsEnqueued = true;
    if (thread->State() == ThreadState::eRunning)
        thread->SetState(ThreadState::eReady);

    s_ExecutionQueue.PushBack(thread);
}
void Scheduler::DequeueThread(Thread* thread)
{
    if (!thread->IsEnqueued()) return;
    ScopedLock guard(s_Lock);

    for (auto& current : s_ExecutionQueue)
        if (&current == thread)
        {
            thread->m_IsEnqueued = false;
            thread->SetState(ThreadState::eDequeued);
        }
}

UnorderedMap<pid_t, Process*>& Scheduler::GetProcessMap()
{
    return s_Processes;
}

Thread* Scheduler::GetNextThread(usize cpuID)
{
    if (s_ExecutionQueue.IsEmpty()) return nullptr;

    auto thread          = s_ExecutionQueue.PopFrontElement();
    thread->m_IsEnqueued = false;

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
            s_BlockedQueue.PushBack(newThread);
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
    Thread* newThread = CPU::GetCurrentThread();
    if (!newThread) newThread = CPU::Current()->Idle;
    if (!IsPreemptionEnabled()) goto reschedule;

    newThread = PickReadyThread();
    SwitchContext(newThread, ctx);

    if (newThread != CPU::Current()->Idle)
        newThread->SetState(ThreadState::eRunning);

reschedule:
    CPU::Reschedule(newThread->Parent()->m_Quantum * 1_ms);
}
