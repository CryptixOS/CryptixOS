/*
 * Created by v1tr10l7 on 14.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/linux/time.h>

enum class ClockID : isize
{
    eRealTime        = CLOCK_REALTIME,
    eRealTimeAlarm   = CLOCK_REALTIME_ALARM,
    eRealTimeCoarse  = CLOCK_REALTIME_COARSE,
    eTai             = CLOCK_TAI,
    eMonotonic       = CLOCK_MONOTONIC,
    eMonotonicCoarse = CLOCK_MONOTONIC_COARSE,
    eMonotonicRaw    = CLOCK_MONOTONIC_RAW,
    eBootTime        = CLOCK_BOOTTIME,
    eBootTimeAlarm   = CLOCK_BOOTTIME_ALARM,
    eProcessCPUTime  = CLOCK_PROCESS_CPUTIME_ID,
    eThreadCPUTime   = CLOCK_THREAD_CPUTIME_ID,
};
