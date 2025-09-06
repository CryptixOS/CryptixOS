/*
 * Created by v1tr10l7 on 02.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Scheduler/Event.hpp>

class WaitQueue : public NonCopyable<WaitQueue>
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
