/*
 * Created by vitriol1744 on 18.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Time/HardwareTimer.hpp>

class HPETComparator final : public HardwareTimer
{
  public:
    ErrorOr<void> Start(u8 vector, TimeStep interval, TimerMode mode) override
    {
        return Error(ENOSYS);
    }
    void          Stop() override {}

    ErrorOr<void> SetFrequency(usize frequency) override
    {
        return Error(ENOSYS);
    }
};
