/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>

#include <Time/Time.hpp>

namespace Time
{
    static timespec s_RealTime;
    static timespec s_Monotonic;

    usize           GetEpoch() { return Arch::GetEpoch(); }
    timespec        GetReal() { return s_RealTime; }
    timespec        GetMonotonic() { return s_Monotonic; }

    void            Tick(usize ns)
    {
        s_RealTime.tv_nsec += ns;
        s_Monotonic.tv_nsec += ns;
    }
}; // namespace Time
