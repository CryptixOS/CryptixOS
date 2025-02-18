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

#include <Memory/PMM.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>
#include <Utility/Spinlock.hpp>

#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/VFS.hpp>

#include <deque>

u8 g_ScheduleVector = 0x20;
namespace Scheduler
{
    namespace
    {
        Spinlock                            s_Lock{};

        Process*                            s_KernelProcess = nullptr;
        Spinlock                            s_ProcessListLock;
        std::unordered_map<pid_t, Process*> s_Processes;
        ProcFs*                             s_ProcFs = nullptr;

        struct ThreadQueue
        {
            mutable Spinlock    Lock;
            std::deque<Thread*> Queue;

            constexpr bool      IsEmpty() const { return Queue.empty(); }

            inline Thread*      Front() const
            {
                ScopedLock guard(Lock);
                return Queue.front();
            }
            inline Thread* Back() const
            {
                ScopedLock guard(Lock);
                return Queue.back();
            }

            inline void PushBack(Thread* t)
            {
                ScopedLock guard(Lock);
                Queue.push_back(t);
            }
            inline void PushFront(Thread* t)
            {
                ScopedLock guard(Lock);
                Queue.push_front(t);
            }

            inline void PopBack()
            {
                ScopedLock guard(Lock);
                Queue.pop_back();
            }
            inline void PopFront()
            {
                ScopedLock guard(Lock);
                Queue.pop_front();
            }

            inline Thread* PopFrontElement()
            {
                ScopedLock guard(Lock);
                return Queue.pop_front_element();
            }
            inline Thread* PopBackElement()
            {
                ScopedLock guard(Lock);
                return Queue.pop_back_element();
            }
        };

        ThreadQueue s_ExecutionQueue;
        ThreadQueue s_WaitQueue;
        ThreadQueue s_ReadyQueue;
        ThreadQueue s_BlockedQueue;

        Thread*     GetNextThread(usize cpuID)
        {
            ScopedLock guard(s_Lock);
            if (s_ExecutionQueue.IsEmpty()) return nullptr;

            auto thread      = s_ExecutionQueue.PopFrontElement();
            thread->enqueued = false;

            return thread;
        }
    } // namespace
    void Schedule(CPUContext* ctx);

    void Initialize()
    {
        s_KernelProcess = Process::CreateKernelProcess();
        LogInfo("Scheduler: Kernel process created");
        LogInfo("Scheduler: Initialized");
    }
    void InitializeProcFs()
    {
        VFS::CreateNode(VFS::GetRootNode(), "/proc", 0755 | S_IFDIR);
        Assert(VFS::Mount(VFS::GetRootNode(), "", "/proc", "procfs"));
        s_ProcFs = reinterpret_cast<ProcFs*>(VFS::GetMountPoints()["/proc"]);

        s_ProcFs->AddProcess(s_KernelProcess);
    }
    void PrepareAP(bool start)
    {
        Process* process    = Process::CreateIdleProcess();

        auto     idleThread = new Thread(
            process, reinterpret_cast<uintptr_t>(Arch::Halt), false);
        idleThread->state       = ThreadState::eReady;
        CPU::GetCurrent()->Idle = idleThread;

        if (start)
        {
            CPU::SetInterruptFlag(true);

            CPU::WakeUp(0, true);
            // CPU::GetCurrent()->Lapic.Start(g_ScheduleVector, 1000,
            //                                Lapic::Mode::eOneshot);
            //  InterruptManager::Unmask(0);

            for (;;) Arch::Halt();
        }
    };

    void Block(Thread* thread) { ToDo(); }
    [[noreturn]]
    void Yield()
    {
        CPU::SetInterruptFlag(true);

        for (;;) Arch::Halt();
    }

    Process* GetKernelProcess() { return s_KernelProcess; }

    Process* CreateProcess(Process* parent, std::string_view name,
                           const Credentials& creds)
    {
        auto       proc = new Process(parent, name, creds);

        ScopedLock guard(s_ProcessListLock);
        s_Processes[proc->GetPid()] = proc;
        s_ProcFs->AddProcess(proc);

        return proc;
    }
    void RemoveProcess(pid_t pid)
    {
        ScopedLock guard(s_ProcessListLock);
        s_Processes.erase(pid);
        s_ProcFs->RemoveProcess(pid);
    }

    bool ValidatePid(pid_t pid)
    {
        ScopedLock guard(s_ProcessListLock);
        return s_Processes.contains(pid);
    }
    Process* GetProcess(pid_t pid)
    {
        ScopedLock guard(s_ProcessListLock);

        auto       it = s_Processes.find(pid);

        return it != s_Processes.end() ? s_Processes[pid] : nullptr;
    }

    Thread* CreateKernelThread(uintptr_t pc, uintptr_t arg, usize runningOn)
    {
        return new Thread(s_KernelProcess, pc, arg, runningOn);
    }

    void EnqueueThread(Thread* thread)
    {
        ScopedLock guard(s_Lock);

        if (thread->enqueued) return;

        thread->enqueued = true;
        thread->state    = ThreadState::eReady;
        s_ExecutionQueue.PushBack(thread);
    }

    void EnqueueNotReady(Thread* thread)
    {
        ScopedLock guard(s_Lock);

        thread->enqueued = true;
        if (thread->state == ThreadState::eRunning)
            thread->state = ThreadState::eReady;

        s_ExecutionQueue.PushBack(thread);
    }

    void Schedule(CPUContext* ctx)
    {
        auto newThread = GetNextThread(CPU::GetCurrent()->ID);
        while (newThread && newThread->state != ThreadState::eReady)
        {
            if (newThread->state == ThreadState::eKilled)
            {
                delete newThread;
                continue;
            }

            EnqueueNotReady(newThread);
            newThread = GetNextThread(CPU::GetCurrent()->ID);
        }

        if (!newThread) newThread = CPU::GetCurrent()->Idle;
        else newThread->state = ThreadState::eRunning;

        auto currentThread = CPU::GetCurrentThread();
        if (currentThread && currentThread->state != ThreadState::eKilled)
        {
            if (currentThread != CPU::GetCurrent()->Idle)
                EnqueueNotReady(currentThread);

            CPU::SaveThread(currentThread, ctx);
        }

        CPU::Reschedule(newThread->parent->m_Quantum * 1_ms);
        CPU::LoadThread(newThread, ctx);

        if (currentThread && currentThread->state == ThreadState::eKilled
            && currentThread != CPU::GetCurrent()->Idle)
            delete currentThread;
    }
}; // namespace Scheduler
