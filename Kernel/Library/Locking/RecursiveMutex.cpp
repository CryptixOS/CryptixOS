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
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/UnixTypes.hpp>
#include <API/Posix/termios.h>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/IntrusiveList.hpp>

module Cryptix:Impl.Library.Locking.RecursiveMutex;

import :Library.Locking.RecursiveMutex;
import :Scheduler.Thread;

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
