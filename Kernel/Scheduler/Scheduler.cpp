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
#include <Scheduler/Spinlock.hpp>
#include <Scheduler/Thread.hpp>

#include <deque>
#include <mutex>

u8 g_ScheduleVector = 0x20;
namespace Scheduler
{
    namespace
    {
        Spinlock            s_Lock{};

        Process*            s_KernelProcess = nullptr;
        std::deque<Thread*> s_ExecutionQueue;
        std::deque<Thread*> queues[2];
        auto                active  = &queues[0];
        auto                expired = &queues[1];

        Thread*             GetNextThread(usize cpuID)
        {
            ScopedLock guard(s_Lock);
            if (s_ExecutionQueue.empty()) return nullptr;
            Thread* t = s_ExecutionQueue.front();
            s_ExecutionQueue.pop_front();
            return t;

            if (active->empty()) std::swap(active, expired);
            if (active->empty()) return nullptr;

            for (auto it = active->rbegin(); it != active->rend(); it++)
            {
                Thread* thread = *it;
                if (thread->runningOn < 0 || thread->runningOn == cpuID || true)
                {
                    active->erase(std::next(it).base());
                    thread->enqueued = false;

                    return thread;
                }
            }
            return nullptr;
        }
    } // namespace
    void Schedule(CPUContext* ctx);

    void Initialize()
    {
        s_KernelProcess
            = new Process("Kernel Process", PrivilegeLevel::ePrivileged);
        LogInfo("Scheduler: Kernel process created");
        LogInfo("Scheduler: Initialized");
    }
    void PrepareAP(bool start)
    {
        CPU::GetCurrent()->TSS.ist[0]
            = ToHigherHalfAddress<uintptr_t>(PMM::AllocatePages<uintptr_t>(
                  CPU::KERNEL_STACK_SIZE / PMM::PAGE_SIZE))
            + CPU::KERNEL_STACK_SIZE;
        IDT::SetIST(14, 2);

        static std::atomic<pid_t> idlePids(-1);

        Process*                  process = new Process;
        process->pid                      = idlePids--;
        process->name                     = "Idle Process for CPU: ";
        process->name += std::to_string(CPU::GetCurrent()->ID);
        process->pageMap = VMM::GetKernelPageMap();

        auto idleThread  = new Thread(
            process, reinterpret_cast<uintptr_t>(Arch::Halt), false);
        idleThread->state       = ThreadState::eReady;
        CPU::GetCurrent()->Idle = idleThread;

        if (start)
        {
            CPU::SetInterruptFlag(true);

            // CPU::GetCurrent()->Lapic.Start(0x20, 20000,
            // Lapic::Mode::eOneshot);
            InterruptManager::Unmask(0);

            for (;;) Arch::Halt();
        }
    };

    [[noreturn]]
    void Yield()
    {
        CPU::SetInterruptFlag(true);

        for (;;) Arch::Halt();
    }

    Process* GetKernelProcess() { return s_KernelProcess; }
    Thread*  CreateKernelThread(uintptr_t pc, uintptr_t arg, usize runningOn)
    {
        return new Thread(s_KernelProcess, pc, arg, runningOn);
    }

    void EnqueueThread(Thread* thread)
    {
        ScopedLock guard(s_Lock);
        if (thread->enqueued) return;

        thread->enqueued = true;
        thread->state    = ThreadState::eReady;
        // expired->push_front(thread);
        s_ExecutionQueue.push_back(thread);
    }

    void EnqueueNotReady(Thread* thread)
    {
        ScopedLock guard(s_Lock);

        thread->enqueued = true;
        if (thread->state == ThreadState::eRunning)
            thread->state = ThreadState::eReady;

        // expired->push_front(thread);
        s_ExecutionQueue.push_back(thread);
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

        CPU::Reschedule(1000);
        CPU::LoadThread(newThread, ctx);

        if (currentThread && currentThread->state == ThreadState::eKilled
            && currentThread != CPU::GetCurrent()->Idle)
            delete currentThread;
    }
}; // namespace Scheduler
