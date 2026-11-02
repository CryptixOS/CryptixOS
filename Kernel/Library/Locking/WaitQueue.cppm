/*
 * Created by v1tr10l7 on 02.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Utility/Optional.hpp>

export module Cryptix:Library.Locking.WaitQueue;

export import :Scheduler.Event;

export class WaitQueue : public NonCopyable<WaitQueue>
{
  public:
    WaitQueue() = default;

    void WakeOne();
    void WakeAll();

    template <typename Predicate>
    inline void Wait(Predicate condition)
    {
        for (;;)
        {
            {
                ScopedLock guard(m_Lock);
                if (condition()) return;
            }

            // Block until an event is triggered
            m_Event.Await();
        }
    }

  private:
    Spinlock m_Lock;
    Event    m_Event;
};
