/*
 * Created by v1tr10l7 on 14.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/Time.hpp>
#include <Scheduler/Event.hpp>

namespace Time
{
    struct Timer : public RefCounted
    {
        explicit Timer()       = default;

        Optional<usize>  Index = NullOpt;
        bool             Fired = false;
        Timestep         When{0};
        Timestep         ReloadValue{0};
        Event            Event;
        Delegate<void()> OnFired;

        void Arm(usize id, Timestep expiration, Timestep reloadValue = 0);
        template <typename F>
        inline void Arm(usize id, Timestep expiration, F fn,
                        Timestep reloadValue = 0)
        {
            OnFired.BindLambda(fn);
            Arm(id, expiration, reloadValue);
        }
        void Disarm();
    };
}; // namespace Time
using Time::Timer;
