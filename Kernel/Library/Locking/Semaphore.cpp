/*
 * Created by v1tr10l7 on 24.06.2025.
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

module Cryptix:Impl.Library.Locking.Semaphore;

import :Library.Locking.Semaphore;

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
