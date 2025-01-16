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
    bool TestAndAcquire()
    {
        return __sync_bool_compare_and_swap(&m_Lock, 0, 1);
    }

    void Acquire(bool disableInterrupts = false)
    {
        if (disableInterrupts)
            m_SavedInterruptState = CPU::SwapInterruptFlag(false);
        volatile usize deadLockCounter = 0;
        for (;;)
        {
            if (TestAndAcquire()) break;
            deadLockCounter += 1;
            if (deadLockCounter >= 100000000) goto deadlock;

            Arch::Pause();
        }

        m_LastAcquirer = __builtin_return_address(0);
        return;

    deadlock:
        Panic("DEADLOCK");
    }
    void Release(bool restoreInterrupts = false)
    {
        m_LastAcquirer = nullptr;
        __sync_bool_compare_and_swap(&m_Lock, 1, 0);

        if (restoreInterrupts) CPU::SetInterruptFlag(m_SavedInterruptState);
        m_SavedInterruptState = false;
    }

  private:
    i32   m_Lock                = 0;
    void* m_LastAcquirer        = nullptr;
    bool  m_SavedInterruptState = false;
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
