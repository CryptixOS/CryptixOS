/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Locking/RecursiveMutex.hpp>
#include <Scheduler/Thread.hpp>

void RecursiveMutex::Lock()
{
    for (;;)
    {
        if (TryLock()) break;

        m_Event.Await();
    }
}
bool RecursiveMutex::TryLock()
{
    Thread*    current = Thread::Current();
    ScopedLock guard(m_Lock);

    if (!m_Locked)
    {
        m_Locked = true;
        m_Owner  = current;
        m_Depth  = 1;
        return true;
    }
    else if (m_Owner == current)
    {
        ++m_Depth;
        return true;
    }

    return false;
}
void RecursiveMutex::Unlock()
{
    ScopedLock guard(m_Lock);
    Assert(m_Locked && m_Owner == Thread::Current());

    if (--m_Depth == 0)
    {
        m_Locked = false;
        m_Owner  = nullptr;
        m_Event.Trigger();
    }
}
