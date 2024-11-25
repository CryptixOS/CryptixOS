/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Scheduler.hpp"

#include "Arch/CPU.hpp"
#include "Arch/InterruptHandler.hpp"
#include "Arch/InterruptManager.hpp"
#include "Arch/x86_64/IDT.hpp"

#include "Memory/PMM.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Thread.hpp"

#include <deque>
#include <mutex>

namespace Scheduler
{
    namespace
    {
        [[maybe_unused]] u8        scheduleVector = 0;
        std::mutex                 lock;

        static std::deque<Thread*> queues[2];
        static auto                active  = &queues[0];
        static auto                expired = &queues[1];

        Thread*                    GetNextThread(usize cpuID)
        {
            std::unique_lock guard(lock);
            if (active->empty()) std::swap(active, expired);
            if (active->empty()) return nullptr;

            for (auto it = active->rbegin(); it != active->rend(); it++)
            {
                Thread* thread = *it;
                if (thread->runningOn < 0 || thread->runningOn == cpuID)
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
        CPU::SetInterruptFlag(true);
        InterruptManager::Unmask(0);
        LogInfo("Scheduler: Initialized");
        for (;;) Arch::Halt();
    }

    void EnqueueThread(Thread* thread)
    {
        std::unique_lock guard(lock);
        if (thread->enqueued) return;
        thread->state = ThreadState::eRunning;
        expired->push_front(thread);
    }
    void EnqueueNotReady(Thread* thread)
    {
        std::unique_lock guard(lock);

        thread->enqueued = true;
        if (thread->state == ThreadState::eRunning)
            thread->state = ThreadState::eReady;

        expired->push_front(thread);
    }

    void Schedule(CPUContext* ctx)
    {
        auto newThread = GetNextThread(CPU::GetCurrent()->id);
        while (newThread && newThread->state != ThreadState::eReady)
        {
            if (newThread->state == ThreadState::eKilled)
            {
                delete newThread;
                continue;
            }

            EnqueueNotReady(newThread);
            newThread = GetNextThread(CPU::GetCurrent()->id);
        }

        if (!newThread) newThread = CPU::GetCurrent()->idle;
        else newThread->state = ThreadState::eRunning;

        auto currentThread = CPU::GetCurrentThread();
        if (currentThread && currentThread->state != ThreadState::eKilled)
        {
            if (currentThread != CPU::GetCurrent()->idle)
                EnqueueNotReady(currentThread);

            CPU::SaveThread(newThread, ctx);
        }

        CPU::LoadThread(newThread, ctx);
        if (currentThread && currentThread->state == ThreadState::eKilled
            && currentThread != CPU::GetCurrent()->idle)
            delete currentThread;
    }
}; // namespace Scheduler
