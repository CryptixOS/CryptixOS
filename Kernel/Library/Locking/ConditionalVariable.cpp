/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Locking/ConditionalVariable.hpp>

void ConditionalVariable::Wait(Mutex& mutex)
{
    Thread* thread = Thread::Current();
    {
        ScopedLock guard(m_Lock);
        m_Event.Listeners.EmplaceBack(thread, 0);
        thread->GetEvents().PushBack(&m_Event);
    }

    mutex.Unlock();
    m_Event.Await();
    mutex.Lock();
}

void ConditionalVariable::NotifyOne()
{
    ScopedLock guard(m_Lock);
    if (!m_Event.Listeners.Empty()) m_Event.Trigger(true);
}
void ConditionalVariable::NotifyAll()
{
    ScopedLock guard(m_Lock);
    m_Event.Trigger(true);
}
