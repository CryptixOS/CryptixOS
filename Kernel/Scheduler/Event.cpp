/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Scheduler/Event.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

static std::optional<usize> CheckPending(std::span<Event*> events)
{
    for (usize i = 0; auto& event : events)
    {
        if (event->Pending > 0)
        {
            --events[i]->Pending;
            return i;
        }
        ++i;
    }
    return std::nullopt;
}

static void AttachListeners(std::span<Event*> events, Thread* thread)
{
    thread->GetEvents().clear();

    for (usize i = 0; const auto& event : events)
    {
        event->Listeners.emplace_back(thread, i);
        thread->GetEvents().push_back(event);
        i++;
    }
}
static void DetachListeners(Thread* thread)
{
    for (const auto& event : thread->GetEvents())
    {
        for (auto it = event->Listeners.begin(); it < event->Listeners.end();
             it++)
        {
            if (it->Thread != thread) continue;
            it = event->Listeners.erase(it);
        }
    }
    thread->GetEvents().clear();
}

static void LockEvents(std::span<Event*> events)
{
    for (auto& event : events) event->Lock.Acquire();
}
static void UnlockEvents(std::span<Event*> events)
{
    for (auto& event : events) event->Lock.Release();
}

std::optional<usize> Event::Await(std::span<Event*> events, bool block)
{
    auto thread   = Thread::GetCurrent();
    bool intState = CPU::SwapInterruptFlag(false);
    LockEvents(events);

    auto i = CheckPending(events);
    if (i.has_value())
    {
        UnlockEvents(events);
        CPU::SetInterruptFlag(intState);
        return i;
    }

    if (!block)
    {
        UnlockEvents(events);
        CPU::SetInterruptFlag(intState);
        return std::nullopt;
    }

    AttachListeners(events, thread);
    UnlockEvents(events);
    CPU::SetInterruptFlag(true);

    Scheduler::Block(thread);

    CPU::SetInterruptFlag(false);
    LockEvents(events);

    DetachListeners(thread);
    UnlockEvents(events);

    CPU::SetInterruptFlag(intState);
    return thread->GetWhich();
}

void Event::Trigger(Event* event, bool drop)
{
    bool intState = CPU::SwapInterruptFlag(false);

    if (event->Listeners.empty())
    {
        if (!drop) ++event->Pending;
        return;
    }

    ScopedLock guard(event->Lock);
    for (const auto& listener : event->Listeners)
    {
        auto thread = listener.Thread;
        thread->SetWhich(listener.Which);
        Scheduler::Unblock(thread);
    }

    event->Listeners.clear();
    CPU::SetInterruptFlag(intState);
}
