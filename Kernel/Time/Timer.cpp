/*
 * Created by v1tr10l7 on 14.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Time/Time.hpp>

namespace Time
{
    void Timer::Arm(usize id, Timestep expiration, Timestep reloadValue)
    {
        Index       = id;
        When        = expiration;
        ReloadValue = reloadValue;
    }
    void Timer::Disarm()
    {
        Index = NullOpt;
        OnFired.Reset();
    }
}; // namespace Time
