/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Utility/Time.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
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
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>

export module Cryptix:Time.Time;

export import :Time.ClockSource;
export import :Time.HardwareTimer;
export import :Time.Timer;

export namespace Time
{
    void           Initialize(DateTime bootTime);
    HardwareTimer* SchedulerTimer();

    ErrorOr<void>  RegisterTimer(HardwareTimer* timer);
    ErrorOr<void>  RegisterClockSource(ClockSource* clock);

    usize          ArmTimer(Ref<Timer> timer, Timestep expiration,
                            Timestep reloadValue = 0);
    template <typename Fn>
    usize ArmTimer(Ref<Timer> timer, Timestep expiration, Fn&& fn,
                   Timestep reloadValue = 0)
    {
        timer->OnFired.BindLambda(fn);
        return ArmTimer(timer, expiration, reloadValue);
    }
    void          DisarmTimer(Ref<Timer> timer);

    Timestep      GetBootTime();
    Timestep      GetTimeSinceBoot();
    Timestep      GetRealTime();
    Timestep      GetMonotonicTime();

    timespec      GetReal();
    timespec      GetMonotonic();

    ErrorOr<void> NanoSleep(usize ns);
    ErrorOr<void> Sleep(const timespec* duration, timespec* remaining);

    void          Tick(usize ns);
} // namespace Time
