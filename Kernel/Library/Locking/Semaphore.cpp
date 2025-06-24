/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Locking/Semaphore.hpp>

void Semaphore::Signal()
{
    ScopedLock guard(m_Lock);
    if (!m_Event.Listeners.Empty()) m_Event.Trigger();
    else ++m_Count;
}
void Semaphore::Wait()
{
    for (;;)
    {
        if (TryWait()) break;

        m_Event.Await();
    }
}
bool Semaphore::TryWait()
{
    ScopedLock guard(m_Lock);
    if (m_Count > 0)
    {
        --m_Count;
        return true;
    }

    return false;
}
