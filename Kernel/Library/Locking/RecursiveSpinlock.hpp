/*
 * Created by v1tr10l7 on 17.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/InterruptGuard.hpp>
#include <Library/Locking/Spinlock.hpp>
#include <Scheduler/Thread.hpp>

/// @brief A spinlock that supports recursive (reentrant) locking by the same
/// CPU/thread.
class RecursiveSpinlock : public NonCopyable<RecursiveSpinlock>
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
class ScopedRecursiveLock final : public NonCopyable<ScopedRecursiveLock> {
public:
    ScopedRecursiveLock(RecursiveSpinlock& lock, bool disableInterrupts = false)
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
