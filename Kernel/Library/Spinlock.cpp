/*
 * Created by v1tr10l7 on 21.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Spinlock.hpp>

#include <Scheduler/Scheduler.hpp>

void Spinlock::Acquire(bool disableInterrupts)
{
    volatile usize deadLockCounter = 0;
    for (;;)
    {
        if (TestAndAcquire()) [[likely]]
        {
            if (disableInterrupts)
                m_SavedInterruptState = CPU::SwapInterruptFlag(false);
            // Scheduler::DisablePreemption();
            break;
        }

        while (AtomicLoad(m_Lock, MemoryOrder::eAtomicRelaxed)
               == LockState::eLocked)
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
    m_LastAcquirer = nullptr;
    AtomicStore(m_Lock, LockState::eUnlocked, MemoryOrder::eAtomicRelease);

    // Scheduler::EnablePreemption();
    if (restoreInterrupts) CPU::SetInterruptFlag(m_SavedInterruptState);
    m_SavedInterruptState = false;
}
