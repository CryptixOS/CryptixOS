/*
 * Created by v1tr10l7 on 02.09.2025.
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

module Cryptix:Impl.Library.Locking.WaitQueue;

import :Library.Locking.WaitQueue;

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
