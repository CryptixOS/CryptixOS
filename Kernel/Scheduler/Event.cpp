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

static Optional<usize> CheckPending(Span<Event*> events)
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
    return NullOpt;
}

static void AttachListeners(Span<Event*> events, Thread* thread)
{
    thread->Events().Clear();

    for (usize i = 0; const auto& event : events)
    {
        event->Listeners.EmplaceBack(thread, i);
        thread->Events().PushBack(event);
        i++;
    }
}
static void DetachListeners(Thread* thread)
{
    for (const auto& event : thread->Events())
    {
        for (auto it = event->Listeners.begin(); it < event->Listeners.end();
             it++)
        {
            if (it->Thread != thread) continue;
            it = event->Listeners.Erase(it);
        }
    }
    thread->Events().Clear();
}

static void LockEvents(Span<Event*> events)
{
    for (auto& event : events) event->Lock.Acquire();
}
static void UnlockEvents(Span<Event*> events)
{
    for (auto& event : events) event->Lock.Release();
}

Optional<usize> Event::Await(Span<Event*> events, bool block)
{
    auto thread   = Thread::Current();
    bool intState = CPU::SwapInterruptFlag(false);
    LockEvents(events);

    auto i = CheckPending(events);
    if (i.HasValue())
    {
        UnlockEvents(events);
        CPU::SetInterruptFlag(intState);
        return i;
    }

    if (!block)
    {
        UnlockEvents(events);
        CPU::SetInterruptFlag(intState);
        return NullOpt;
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
    return thread->Which();
}

void Event::Trigger(Event* event, bool drop)
{
    bool intState = CPU::SwapInterruptFlag(false);

    if (event->Listeners.Empty())
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

    event->Listeners.Clear();
    CPU::SetInterruptFlag(intState);
}
