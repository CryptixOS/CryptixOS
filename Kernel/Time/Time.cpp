/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>
#include <Debug/Assertions.hpp>

#include <Library/Logger.hpp>
#include <Prism/Utility/Time.hpp>

#include <Time/Time.hpp>

namespace Time
{
    namespace
    {
        std::vector<HardwareTimer*> s_HardwareTimers;
        HardwareTimer*              s_SchedulerTimer = nullptr;

        std::optional<i64>          s_BootTime       = std::nullopt;
        timespec                    s_RealTime;
        timespec                    s_Monotonic;
    } // namespace

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
        s_SchedulerTimer = s_HardwareTimers.back();
    }

    HardwareTimer* GetSchedulerTimer() { return s_SchedulerTimer; }

    usize          GetEpoch() { return Arch::GetEpoch(); }
    timespec       GetReal() { return s_RealTime; }
    timespec       GetMonotonic() { return s_Monotonic; }

    void           Tick(usize ns)
    {
        s_RealTime.tv_nsec += ns;
        s_Monotonic.tv_nsec += ns;
    }
}; // namespace Time
