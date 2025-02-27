/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

#include <Prism/Types.hpp>
#include <Time/HardwareTimer.hpp>

namespace Time
{
    void           Initialize();
    HardwareTimer* GetSchedulerTimer();

    usize          GetEpoch();
    timespec       GetReal();
    timespec       GetMonotonic();

    void           Tick(usize ns);
} // namespace Time
