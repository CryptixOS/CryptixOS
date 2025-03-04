/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Scheduler/Event.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <array>
#include <span>

class Mutex
{
  public:
    Mutex() = default;

    bool TryLock()
    {
        if (!m_Locked.exchange(true, std::memory_order_acquire))
        {
            m_Holder = Thread::GetCurrent();
            return true;
        }

        return false;
    }
    void Lock()
    {
        if (TryLock())
        {
            m_Holder = Thread::GetCurrent();
            return;
        };

        for (;;)
        {
            if (!TryLock())
            {
                std::array<Event*, 1> evs = {&m_LockEvent};
                Event::Await(evs);
                Scheduler::Block(Thread::GetCurrent());
            }

            if (TryLock()) break;
        }

        m_Holder = Thread::GetCurrent();
    }
    void Unlock()
    {
        Assert(Thread::GetCurrent() == m_Holder);
        m_Holder = nullptr;

        m_Locked.exchange(false, std::memory_order_release);
        Event::Trigger(&m_LockEvent);
    }

  private:
    std::atomic<bool>  m_Locked             = false;
    std::atomic<usize> m_WaitingThreadCount = 0;
    Event              m_LockEvent;
    Thread*            m_Holder = nullptr;
};
