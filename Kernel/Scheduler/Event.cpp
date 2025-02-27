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

static isize CheckPending(Event** events, usize eventCount)
{
    for (usize i = 0; i < eventCount; i++)
        if (events[i]->Pending > 0)
        {
            --events[i]->Pending;
            return i;
        }
    return -1;
}

static void AttachListeners(Event** events, usize eventCount, Thread* thread)
{
    thread->AttachedEventsI = 0;

    for (usize i = 0; i < eventCount; i++)
    {
        Event* event = events[i];
        if (event->ListenersI == MAX_LISTENERS)
            Panic("Event Listeners exhausted");

        EventListener* listener = &event->Listeners[event->ListenersI++];
        listener->Thread        = thread;
        listener->Which         = i;

        if (thread->AttachedEventsI == MAX_EVENTS)
            Panic("Listening on too many events");
        thread->AttachedEvents[thread->AttachedEventsI++] = event;
    }
}
static void DetachListeners(Thread* thread);

static void LockEvents(Event** events, usize eventCount)
{
    for (usize i = 0; i < eventCount; i++) events[i]->Lock.Acquire();
}
static void UnlockEvents(Event** events, usize eventCount)
{
    for (usize i = 0; i < eventCount; i++) events[i]->Lock.Release();
}

isize Event::Await(Event** events, usize eventCount, bool block)
{
    isize   ret           = -1;

    Thread* thread        = CPU::GetCurrentThread();

    bool    interruptFlag = CPU::SwapInterruptFlag(false);
    LockEvents(events, eventCount);

    isize i = CheckPending(events, eventCount);
    if (i != -1)
    {
        ret = i;
        UnlockEvents(events, eventCount);
        goto cleanup;
    }

    if (!block)
    {
        UnlockEvents(events, eventCount);
        goto cleanup;
    }

    AttachListeners(events, eventCount, thread);
    Scheduler::Block(thread);
    UnlockEvents(events, eventCount);

    Scheduler::Yield();
    CPU::SetInterruptFlag(false);

    if (thread->EnqueuedBySignal()) goto cleanup2;
    ret = thread->GetWhichEvent();

cleanup2:
    LockEvents(events, eventCount);
    DetachListeners(thread);
    UnlockEvents(events, eventCount);
cleanup:
    CPU::SetInterruptFlag(interruptFlag);
    return ret;
}
