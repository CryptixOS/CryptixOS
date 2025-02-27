/*
 * Created by v1tr10l7 on 18.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>
#include <Time/TimeStep.hpp>

enum class TimerMode
{
    eOneShot,
    ePeriodic,
};

class HardwareTimer
{
  public:
    HardwareTimer()          = default;
    virtual ~HardwareTimer() = default;

    virtual ErrorOr<void> Start(TimerMode mode, TimeStep interval, u8 vector)
        = 0;
    virtual void          Stop()                        = 0;

    virtual ErrorOr<void> SetFrequency(usize frequency) = 0;
};
