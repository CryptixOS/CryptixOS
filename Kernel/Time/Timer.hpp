/*
 * Created by v1tr10l7 on 14.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Utility/Time.hpp>
#include <Scheduler/Event.hpp>

namespace Time
{
    struct Timer
    {
        explicit Timer() = default;
        explicit Timer(Timestep when)
            : When(when)
        {
            Arm();
        }
        inline void Fire(Timestep when)
        {
            When = when;
            Arm();
        }

        Optional<usize> Index = NullOpt;
        bool            Fired = false;
        Timestep        When{0};
        Event           Event;

        void            Arm();
        void            Disarm();
    };
}; // namespace Time
using Time::Timer;
