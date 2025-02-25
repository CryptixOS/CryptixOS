/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#pragma once

#include <Common.hpp>

#include <Arch/Arch.hpp>

namespace CPU
{
    bool SwapInterruptFlag(bool);
}

class Spinlock
{
  public:
    inline bool Test() { return !m_Lock.load(); }
    inline bool TestAndAcquire()
    {
        return !m_Lock.exchange(true, std::memory_order_acquire);
    }

    inline void Acquire(bool disableInterrupts = false)
    {
        if (disableInterrupts)
            m_SavedInterruptState = CPU::SwapInterruptFlag(false);
        volatile usize deadLockCounter = 0;
        for (;;)
        {
            if (TestAndAcquire()) break;

            while (m_Lock.load(std::memory_order_relaxed))
            {
                deadLockCounter += 1;
                if (deadLockCounter >= 100000000) goto deadlock;

                Arch::Pause();
            }
        }

        m_LastAcquirer = __builtin_return_address(0);
        return;

    deadlock:
        earlyPanic("DEADLOCK");
    }
    inline void Release(bool restoreInterrupts = false)
    {
        m_LastAcquirer = nullptr;
        m_Lock.store(false, std::memory_order_release);

        if (restoreInterrupts) CPU::SetInterruptFlag(m_SavedInterruptState);
        m_SavedInterruptState = false;
    }

  private:
    std::atomic_bool m_Lock                = false;
    void*            m_LastAcquirer        = nullptr;
    bool             m_SavedInterruptState = false;
};

class ScopedLock final
{
  public:
    ScopedLock(Spinlock& lock, bool disableInterrupts = false)
        : m_Lock(lock)
        , m_RestoreInterrupts(disableInterrupts)
    {
        lock.Acquire(disableInterrupts);
    }
    ~ScopedLock() { m_Lock.Release(m_RestoreInterrupts); }

  private:
    Spinlock& m_Lock;
    bool      m_RestoreInterrupts = false;
};
