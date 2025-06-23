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

class Mutex
{
  public:
    Mutex() = default;

    bool TryLock()
    {
        if (!m_Locked.Exchange(true, MemoryOrder::eAtomicAcquire))
        {
            m_Holder = Thread::GetCurrent();
            return true;
        }

        return false;
    }
    void Lock()
    {
        if (TryLock()) goto lock_success;

        for (;;)
        {
            if (TryLock()) break;

            m_LockEvent.Await(true);
        }

    lock_success:
        m_Holder = Thread::GetCurrent();
    }
    void Unlock()
    {
        Assert(Thread::GetCurrent() == m_Holder);
        m_Holder = nullptr;

        m_Locked.Exchange(false, MemoryOrder::eAtomicRelease);
        Event::Trigger(&m_LockEvent);
    }

  private:
    Atomic<bool>  m_Locked             = false;
    Atomic<usize> m_WaitingThreadCount = 0;
    Event         m_LockEvent;
    Thread*       m_Holder = nullptr;
};
