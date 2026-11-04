/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Arch/Arch.hpp>
#include <Compiler.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
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
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Algorithm/Find.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>

module Cryptix:Impl.Time.Time;

import :Arch.Arch;
import :Arch.CPU;
import :Boot.CommandLine;
import :Debug.Assertions;
import :Drivers.Core.DeviceManager;
import :Library.Locking.Spinlock;
import :Library.Logger;
import :Time.Time;

namespace Time
{
    namespace
    {
        HardwareTimer::List s_HardwareTimers;
        ClockSource::List   s_ClockSources;
        HardwareTimer*      s_SchedulerTimer = nullptr;

        Timestep            s_BootTime;
        Timestep            s_RealTime;
        Timestep            s_Monotonic;

        Vector<Ref<Timer>>   s_ArmedTimers;
        Spinlock            s_TimersLock;
    } // namespace

    void Initialize(DateTime dateAtBoot)
    {
        s_BootTime = dateAtBoot.ToEpoch() * 1'000'000'000;
        LogInfo("Time: Boot Date: {}", dateAtBoot);

        auto now    = static_cast<usize>(Arch::GetEpoch());
        s_RealTime  = now * 1'000'000'000;
        s_Monotonic = s_RealTime;
        Assert(s_HardwareTimers.Size() > 0);

        LogInfo("Time: Detected {} timers", s_HardwareTimers.Size());
        for (auto timer : s_HardwareTimers) timer->Stop();

        auto cpuLocalTimer
            = FindIf(s_HardwareTimers.begin(), s_HardwareTimers.end(),
                     [](auto it) -> bool { return it->IsCPULocal(); });
        auto nonCpuLocalTimer
            = FindIf(s_HardwareTimers.begin(), s_HardwareTimers.end(),
                     [](auto it) -> bool { return !it->IsCPULocal(); });

        auto requestedTimer = CommandLine::String("x86.sched_timer");
        LogTrace("Time: Requested `{}` timer for scheduling", requestedTimer);

        if ((requestedTimer == "lapic"_sv || requestedTimer.Empty())
            && cpuLocalTimer != s_HardwareTimers.end())
            s_SchedulerTimer = *cpuLocalTimer;
        else if (nonCpuLocalTimer != s_HardwareTimers.end())
            s_SchedulerTimer = *nonCpuLocalTimer;
        else s_SchedulerTimer = s_HardwareTimers.Head();

        LogInfo("Time: Using `{}` as a timer for scheduling",
                s_SchedulerTimer->ModelString());

        for (auto timer : s_HardwareTimers)
            if (timer != s_SchedulerTimer)
                timer->Start(TimerMode::ePeriodic, 1000);
    }

    HardwareTimer* SchedulerTimer() { return s_SchedulerTimer; }

    ErrorOr<void>  RegisterTimer(HardwareTimer* timer)
    {
        auto found
            = FindIf(s_HardwareTimers.begin(), s_HardwareTimers.end(),
                     [timer](auto it) -> bool
                     { return it->ModelString() == timer->ModelString(); });
        if (found != s_HardwareTimers.end()) return Error(EEXIST);

        s_HardwareTimers.PushBack(timer);
        DeviceManager::RegisterCharDevice(timer);

        LogTrace("Time: Registered hardware timer => {}", timer->ModelString());
        return {};
    }
    ErrorOr<void> RegisterClockSource(ClockSource* clock)
    {
        auto found = FindIf(s_ClockSources.begin(), s_ClockSources.end(),
                            [clock](auto it) -> bool
                            { return it->Name() == clock->Name(); });
        if (found != s_ClockSources.end()) return Error(EEXIST);

        s_ClockSources.PushBack(clock);
        DeviceManager::RegisterCharDevice(clock);

        LogTrace("Time: Registered hardware timer => {}", clock->Name());
        return {};
    }

    usize ArmTimer(Ref<Timer> timer, Timestep expiration, Timestep reloadValue)
    {
        ScopedLock guard(s_TimersLock, true);

        auto       id = s_ArmedTimers.Size();
        timer->Arm(id, expiration, reloadValue);
        s_ArmedTimers.EmplaceBack(timer);
        return id;
    }
    void DisarmTimer(::Ref<Timer> timer)
    {
        ScopedLock guard(s_TimersLock, true);
        auto       it = s_ArmedTimers.begin();
        for (; it != s_ArmedTimers.end(); it++)
            if (*it == timer) break;

        if (it != s_ArmedTimers.end()) s_ArmedTimers.Erase(it);
        timer->Disarm();
    }

    Timestep GetBootTime() { return s_BootTime; }
    Timestep GetTimeSinceBoot() { return GetRealTime() - s_BootTime; }
    Timestep GetRealTime() { return s_RealTime; }
    Timestep GetMonotonicTime() { return s_Monotonic; }

    timespec GetReal()
    {
        timespec real;
        real.tv_sec  = s_RealTime.Seconds();
        real.tv_nsec = s_RealTime.Nanoseconds();

        return real;
    }
    timespec GetMonotonic()
    {

        timespec mono;
        mono.tv_sec  = s_Monotonic.Seconds();
        mono.tv_nsec = s_Monotonic.Nanoseconds();

        return mono;
    }

    ErrorOr<void> NanoSleep(usize ns)
    {
        ::Ref<Timer> timer = CreateRef<Timer>();
        ArmTimer(timer, ns);

        timer->Event.Await(true);
        return {};
    }
    ErrorOr<void> Sleep(const timespec* duration, timespec* remaining)
    {
        usize ns = static_cast<usize>(duration->tv_sec) * 1'000'000'000;
        if (ns == 0) ns = static_cast<usize>(duration->tv_nsec);

        ::Ref<Timer> timer = CreateRef<Timer>();
        ArmTimer(timer, ns);

        if (!timer->Event.Await(true))
        {
            if (remaining)
                *remaining = {static_cast<isize>(timer->When.Seconds()),
                              static_cast<isize>(timer->When.Nanoseconds())};
            DisarmTimer(timer);
            return Error(EINTR);
        }

        DisarmTimer(timer);
        return {};
    }

    void Tick(usize ns)
    {
        auto highResClock = CPU::HighResolutionClock();

        if (highResClock)
        {
            auto maybeNs = highResClock->Now();
            auto prev    = s_RealTime;

            if (maybeNs)
            {
                s_RealTime = s_RealTime = *maybeNs;
                ns                      = s_RealTime - prev;
            }
        }
        else
        {
            s_RealTime += ns;
            s_Monotonic += ns;
        }

        if (s_TimersLock.TestAndAcquire())
        {
            Assert(!s_TimersLock.Test());
            for (auto& timer : s_ArmedTimers)
            {
                if (timer->Fired) continue;

                if (ns >= timer->When.Nanoseconds()) timer->When = 0_ns;
                else timer->When -= ns;
                if (timer->When != 0_ns) continue;

                if (timer->OnFired) timer->OnFired();
                else timer->Event.Trigger(false);
                timer->Fired = true;

                // if (timer->ReloadValue) ArmTimer(timer, timer->ReloadValue);
            }

            s_TimersLock.Release();
        }
    }
}; // namespace Time
