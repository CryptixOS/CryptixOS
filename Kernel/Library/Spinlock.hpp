/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/Arch.hpp>
#include <Arch/x86_64/Atomic.hpp>
#include <Debug/Panic.hpp>

namespace CPU
{
    bool SwapInterruptFlag(bool);
    void SetInterruptFlag(bool);
} // namespace CPU

#include <Prism/Core/Platform.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>

#ifdef PRISM_ARCH_X86_64
inline void Pause() { __asm__ volatile("pause"); }
#elifdef PRISM_ARCH_AARCH64
inline void Pause() { __asm__ volatile("isb" ::: "memory"); }
#endif

class Spinlock
{
    enum class LockState : u32
    {
        eUnlocked = 0,
        eLocked   = 1,
    };

  public:
    CTOS_ALWAYS_INLINE bool Test()
    {
        return AtomicLoad(m_Lock, MemoryOrder::eAtomicRelaxed)
            == LockState::eUnlocked;
    }
    CTOS_ALWAYS_INLINE bool TestAndAcquire()
    {
        LockState expected = LockState::eUnlocked;
        return AtomicCompareExchange(&m_Lock, &expected, LockState::eLocked,
                                     false, MemoryOrder::eAtomicAcquire,
                                     MemoryOrder::eAtomicRelaxed);
        return CompareExchange<LockState>(&m_Lock, LockState::eUnlocked,
                                          LockState::eLocked);
    }

    void Acquire(bool disableInterrupts = false);
    void Release(bool restoreInterrupts = false);

  private:
    LockState m_Lock                = LockState::eUnlocked;
    void*     m_LastAcquirer        = nullptr;
    bool      m_SavedInterruptState = false;
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
