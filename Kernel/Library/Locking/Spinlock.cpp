/*
 * Created by v1tr10l7 on 21.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Locking/Spinlock.hpp>

#include <Scheduler/Scheduler.hpp>

void Spinlock::Acquire(bool disableInterrupts)
{
    volatile usize deadLockCounter = 0;
    // Disable interrupts *before* taking the lock: an interrupt between the
    // two, whose handler takes this lock too, would spin on it forever
    bool           interruptState  = false;
    if (disableInterrupts) interruptState = CPU::SwapInterruptFlag(false);
    for (;;)
    {
        if (TestAndAcquire()) [[likely]]
        {
            if (disableInterrupts) m_SavedInterruptState = interruptState;
            // Scheduler::DisablePreemption();
            break;
        }

        while (m_Lock.Load(MemoryOrder::eRelaxed) == LockState::eLocked)
        {
            deadLockCounter += 1;
            if (deadLockCounter >= 100000000) goto deadlock;

            Pause();
        }
    }

    m_LastAcquirer = __builtin_return_address(0);
    return;

deadlock:
    PrismPanic("DEADLOCK");
}

void Spinlock::Release(bool restoreInterrupts)
{
    // Read the saved state while still holding the lock: once it's released
    // another cpu may take it and overwrite the field. Only an
    // interrupt-restoring release consumes it, so a plain Release()/Acquire()
    // pair inside a ScopedLock(lock, true) can't wipe what the outer release
    // restores.
    bool state = m_SavedInterruptState;
    if (restoreInterrupts) m_SavedInterruptState = false;

    m_LastAcquirer = nullptr;
    m_Lock.Store(LockState::eUnlocked, MemoryOrder::eRelease);

    // Scheduler::EnablePreemption();
    if (restoreInterrupts) CPU::SetInterruptFlag(state);
}
