/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>
#include <Debug/Assertions.hpp>

#include <Library/Logger.hpp>
#include <Prism/Spinlock.hpp>

#include <Prism/Utility/Time.hpp>

#include <Scheduler/Event.hpp>
#include <Time/Time.hpp>

namespace Time
{
    struct Timer
    {
        Timer(timespec when)
            : When(when)
        {
            Arm();
        }

        std::optional<usize> Index = std::nullopt;
        bool                 Fired = false;
        timespec             When  = {0, 0};
        Event                Event;

        void                 Arm();
        void                 Disarm();
    };

    namespace
    {
        std::vector<HardwareTimer*> s_HardwareTimers;
        HardwareTimer*              s_SchedulerTimer = nullptr;

        std::optional<i64>          s_BootTime       = std::nullopt;
        timespec                    s_RealTime;
        timespec                    s_Monotonic;

        std::deque<Timer*>          s_ArmedTimers;
        Spinlock                    s_TimersLock;
    } // namespace

    void Timer::Arm()
    {
        ScopedLock guard(s_TimersLock);

        Index = s_ArmedTimers.size();
        s_ArmedTimers.push_back(this);
    }
    void Timer::Disarm()
    {
        ScopedLock guard(s_TimersLock);
        auto       it = s_ArmedTimers.begin();
        for (; it != s_ArmedTimers.end(); it++)
            if (*it == this) break;

        if (it != s_ArmedTimers.end()) s_ArmedTimers.erase(it);
        Index = std::nullopt;
    }

    void Initialize()
    {
        s_BootTime = BootInfo::GetDateAtBoot();
        DateTime dateAtBoot(s_BootTime.value());
        LogInfo("Time: Boot Date: {}", dateAtBoot);

        LogTrace("Time: Probing available timers...");
        Arch::ProbeTimers(s_HardwareTimers);
        Assert(s_HardwareTimers.size() > 0);

        LogInfo("Time: Detected {} timers", s_HardwareTimers.size());
        for (usize i = 0; auto& timer : s_HardwareTimers)
            LogInfo("Time: Timer[{}] = '{}'", i++, timer->GetModelString());

        s_SchedulerTimer = s_HardwareTimers.front();
    }

    HardwareTimer* GetSchedulerTimer() { return s_SchedulerTimer; }

    usize          GetEpoch() { return Arch::GetEpoch(); }
    timespec       GetReal() { return s_RealTime; }
    timespec       GetMonotonic() { return s_Monotonic; }

    ErrorOr<void>  NanoSleep(usize ns)
    {
        isize    ins      = static_cast<isize>(ns);

        timespec duration = {.tv_sec = ins / 1'000'000'000, .tv_nsec = ins};

        Timer*   timer    = new Timer(duration);
        timer->Event.Await(true);
        timer->Disarm();

        delete timer;
        return {};
    }
    ErrorOr<void> Sleep(const timespec* duration, timespec* remaining)
    {
        LogDebug("Sleeping");
        Timer* timer = new Timer(*duration);
        if (!timer->Event.Await(true))
        {
            if (remaining) *remaining = timer->When;
            timer->Disarm();
            return Error(EINTR);
        }

        timer->Disarm();
        delete timer;
        return {};
    }

    void Tick(usize ns)
    {
        s_RealTime.tv_nsec += ns;
        s_Monotonic.tv_nsec += ns;

        if (s_TimersLock.TestAndAcquire())
        {
            for (auto& timer : s_ArmedTimers)
            {
                if (timer->Fired) continue;

                timer->When -= timespec(0, ns);
                if (timer->When == timespec(0, 0))
                {
                    timer->Event.Trigger(false);
                    timer->Fired = true;
                }
            }

            s_TimersLock.Release();
        }
    }
}; // namespace Time
