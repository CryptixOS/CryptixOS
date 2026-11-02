/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
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
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
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
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/Singleton.hpp>
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>

module Cryptix:Impl.Scheduler.Event;

import :Arch.CPU;
import :Scheduler.Event;
import :Scheduler.Scheduler;
import :Scheduler.Thread;

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
