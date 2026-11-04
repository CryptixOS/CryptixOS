/*
 * Created by v1tr10l7 on 21.04.2025.
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
#include <API/UnixTypes.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/Singleton.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
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
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>

module Cryptix:Impl.Library.Locking.Spinlock;

import :Library.Locking.Spinlock;
import :Scheduler.Scheduler;

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
    m_LastAcquirer = nullptr;
    m_Lock.Store(LockState::eUnlocked, MemoryOrder::eRelease);

    // Scheduler::EnablePreemption();
    if (restoreInterrupts) CPU::SetInterruptFlag(m_SavedInterruptState);
    m_SavedInterruptState = false;
}
