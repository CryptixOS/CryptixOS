/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
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

export module Cryptix:Library.Locking.Spinlock;

export import :Arch.Arch;
#if defined(CTOS_TARGET_X86_64)
export import :Arch.x86_64.Atomic;
#endif
export import :Debug.KernelPanic;

#ifdef CTOS_TARGET_X86_64
export inline void Pause() { __asm__ volatile("pause"); }
#elifdef CTOS_TARGET_AARCH64
export inline void Pause() { __asm__ volatile("isb" ::: "memory"); }
#endif

export namespace CPU
{
    bool SwapInterruptFlag(bool);
    void SetInterruptFlag(bool);
} // namespace CPU

export class Spinlock : public NonCopyable<Spinlock>, public NonMovable<Spinlock>
{
    enum class LockState : u32
    {
        eUnlocked = 0,
        eLocked   = 1,
    };

  public:
    CTOS_ALWAYS_INLINE bool Test()
    {
        return m_Lock.Load(MemoryOrder::eRelaxed) == LockState::eUnlocked;
    }
    CTOS_ALWAYS_INLINE bool TestAndAcquire()
    {
        LockState expected = LockState::eUnlocked;
        return m_Lock.CompareExchange(expected, LockState::eLocked, false,
                                      MemoryOrder::eAcquire,
                                      MemoryOrder::eRelaxed);
    }

    void Acquire(bool disableInterrupts = false);
    void Release(bool restoreInterrupts = false);

  private:
    Atomic<LockState> m_Lock                = LockState::eUnlocked;
    void*             m_LastAcquirer        = nullptr;
    bool              m_SavedInterruptState = false;
};

export class ScopedLock final : public NonCopyable<ScopedLock>
{
  public:
    ScopedLock()                        = delete;
    ScopedLock& operator=(ScopedLock&&) = delete;

    explicit ScopedLock(Spinlock& lock, bool disableInterrupts = false)
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

export class SpinLockPolicy
{
  public:
    void                      Init() {}
    CTOS_NODISCARD ScopedLock Lock() { return ScopedLock(m_Lock); }

  private:
    Spinlock m_Lock;
};
