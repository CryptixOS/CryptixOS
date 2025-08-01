/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>
#include <Arch/CPU.hpp>

#include <Debug/Assertions.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Library/Logger.hpp>

#include <Prism/Algorithm/Find.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Time.hpp>

#include <Scheduler/Event.hpp>
#include <Time/Time.hpp>

namespace Time
{
    struct Timer
    {
        explicit Timer(Timestep when)
            : When(when)
        {
            Arm();
        }

        Optional<usize> Index = NullOpt;
        bool            Fired = false;
        Timestep        When{0};
        Event           Event;

        void            Arm();
        void            Disarm();
    };

    namespace
    {
        HardwareTimer::List s_HardwareTimers;
        ClockSource::List   s_ClockSources;
        HardwareTimer*      s_SchedulerTimer = nullptr;

        Timestep            s_BootTime;
        Timestep            s_RealTime;
        Timestep            s_Monotonic;

        Deque<Timer*>       s_ArmedTimers;
        Spinlock            s_TimersLock;
    } // namespace

    void Timer::Arm()
    {
        ScopedLock guard(s_TimersLock);

        Index = s_ArmedTimers.Size();
        s_ArmedTimers.PushBack(this);
    }
    void Timer::Disarm()
    {
        ScopedLock guard(s_TimersLock);
        auto       it = s_ArmedTimers.begin();
        for (; it != s_ArmedTimers.end(); it++)
            if (*it == this) break;

        if (it != s_ArmedTimers.end()) s_ArmedTimers.Erase(it);
        Index = NullOpt;
    }

    void Initialize(DateTime dateAtBoot)
    {
        s_BootTime = dateAtBoot.ToEpoch() * 1'000'000'000;
        LogInfo("Time: Boot Date: {}", dateAtBoot);

        auto now    = static_cast<usize>(Arch::GetEpoch());
        s_RealTime  = now * 1'000'000'000;
        s_Monotonic = s_RealTime;
        Assert(s_HardwareTimers.Size() > 0);

        LogInfo("Time: Detected {} timers", s_HardwareTimers.Size());
        auto cpuLocalTimer
            = FindIf(s_HardwareTimers.begin(), s_HardwareTimers.end(),
                     [](auto it) -> bool { return it->IsCPULocal(); });
        s_SchedulerTimer = cpuLocalTimer != s_HardwareTimers.end()
                             ? *cpuLocalTimer
                             : s_HardwareTimers.Head();
    }

    HardwareTimer* GetSchedulerTimer() { return s_SchedulerTimer; }

    ErrorOr<void>  RegisterTimer(HardwareTimer* timer)
    {
        auto found
            = FindIf(s_HardwareTimers.begin(), s_HardwareTimers.end(),
                     [timer](auto it) -> bool
                     { return it->ModelString() == timer->ModelString(); });
        if (found != s_HardwareTimers.end()) return Error(EEXIST);

        LogTrace("Time: Registered hardware timer => {}", timer->ModelString());
        s_HardwareTimers.PushBack(timer);
        return {};
    }
    ErrorOr<void> RegisterClockSource(ClockSource* clock)
    {
        auto found = FindIf(s_ClockSources.begin(), s_ClockSources.end(),
                            [clock](auto it) -> bool
                            { return it->Name() == clock->Name(); });
        if (found != s_ClockSources.end()) return Error(EEXIST);

        LogTrace("Time: Registered hardware timer => {}", clock->Name());
        s_ClockSources.PushBack(clock);
        return {};
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
        Timer* timer = new Timer(ns);
        timer->Event.Await(true);
        timer->Disarm();

        delete timer;
        return {};
    }
    ErrorOr<void> Sleep(const timespec* duration, timespec* remaining)
    {
        LogDebug("Sleeping");
        usize ns = static_cast<usize>(duration->tv_sec) * 1'000'000'000;
        if (ns == 0) ns = static_cast<usize>(duration->tv_nsec);

        Timer* timer = new Timer(ns);

        if (!timer->Event.Await(true))
        {
            if (remaining)
                *remaining = {static_cast<isize>(timer->When.Seconds()),
                              static_cast<isize>(timer->When.Nanoseconds())};
            timer->Disarm();
            return Error(EINTR);
        }

        timer->Disarm();
        delete timer;
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
            for (auto& timer : s_ArmedTimers)
            {
                if (timer->Fired) continue;

                if (ns >= timer->When.Nanoseconds()) timer->When = 0_ns;
                else timer->When -= ns;
                if (timer->When == 0_ns)
                {
                    timer->Event.Trigger(false);
                    timer->Fired = true;
                }
            }

            s_TimersLock.Release();
        }
    }
}; // namespace Time
