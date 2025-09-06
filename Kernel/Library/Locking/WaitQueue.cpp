/*
 * Created by v1tr10l7 on 02.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Locking/WaitQueue.hpp>

void WaitQueue::WakeOne()
{
    ScopedLock guard(m_Lock);
    if (!m_Event.Listeners.Empty())
        m_Event.Trigger(true);   // drop=true → don’t accumulate Pending
    else m_Event.Trigger(false); // accumulate Pending if no listeners
}
void WaitQueue::WakeAll()
{
    ScopedLock guard(m_Lock);
    m_Event.Trigger(false); // wakes all listeners
}
