/*
 * Created by v1tr10l7 on 30.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Locking/Mutex.hpp>
#include <Scheduler/Thread.hpp>

bool Mutex::TryLock()
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
void Mutex::Lock()
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
void Mutex::Unlock()
{
    ScopedLock guard(m_Lock);
    Assert(m_Locked && Thread::Current() == m_Holder);

    m_Locked = false;
    m_Holder = nullptr;

    Event::Trigger(&m_LockEvent);
}
