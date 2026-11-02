/*
 * Created by v1tr10l7 on 17.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Utility/Optional.hpp>

export module Cryptix:Library.Locking.RecursiveSpinlock;

export import :Arch.InterruptGuard;
export import :Library.Locking.Spinlock;
export import :Scheduler.Thread;

/// @brief A spinlock that supports recursive (reentrant) locking by the same
/// CPU/thread.
export class RecursiveSpinlock : public NonCopyable<RecursiveSpinlock>
{
  public:
    RecursiveSpinlock() = default;

    void Acquire(bool disableInterrupts = false)
    {
        if (disableInterrupts) InterruptGuard guard(true);
        // You need a way to identify
        // thread/core
        auto current = Thread::Current();
        if (m_Owner == current)
        {
            // Recursive acquisition
            ++m_RecursionCount;
            return;
        }
        // Not the owner yet; acquire base spinlock
        m_Lock.Acquire(disableInterrupts);
        m_Owner          = current;
        m_RecursionCount = 1;
    }

    void Release(bool restoreInterrupts = false)
    {
        auto current = Thread::Current();
        Assert(m_Owner == current && m_RecursionCount > 0);

        --m_RecursionCount;

        if (m_RecursionCount == 0)
        {
            m_Owner = nullptr;
            m_Lock.Release(restoreInterrupts);
        }
    }

    bool IsHeldByCurrentThread() const { return m_Owner == Thread::Current(); }

  private:
    Spinlock m_Lock;
    Thread*  m_Owner          = nullptr;
    usize    m_RecursionCount = 0;
};

/**
 * @brief RAII helper for RecursiveSpinlock.
 *
 * Ensures the lock is released when out of scope, preserving recursion depth.
 */
export class ScopedRecursiveLock final : public NonCopyable<ScopedRecursiveLock> {
public:
    explicit ScopedRecursiveLock(RecursiveSpinlock& lock, bool disableInterrupts = false)
        : m_Lock(lock), m_DisableInterrupts(disableInterrupts)
    {
        m_Lock.Acquire(m_DisableInterrupts);
    }

    ~ScopedRecursiveLock()
    {
        m_Lock.Release(/*restoreInterrupts=*/m_DisableInterrupts);
    }

private:
    RecursiveSpinlock& m_Lock;
    bool               m_DisableInterrupts{ false };
};
