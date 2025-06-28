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

class Mutex : public NonCopyable<Mutex>
{
  public:
    Mutex() = default;

    bool TryLock()
    {
        ScopedLock guard(m_Lock);
        if (!m_Locked)
        {
            m_Locked = true;
            m_Holder = Thread::Current();
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
        m_Holder = Thread::Current();
    }
    void Unlock()
    {
        ScopedLock guard(m_Lock);
        Assert(m_Locked && Thread::Current() == m_Holder);

        m_Locked = false;
        m_Holder = nullptr;

        Event::Trigger(&m_LockEvent);
    }

  private:
    Spinlock m_Lock;
    bool     m_Locked = false;
    Thread*  m_Holder = nullptr;
    Event    m_LockEvent;
};
