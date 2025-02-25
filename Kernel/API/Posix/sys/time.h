/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>

using time_t      = isize;
using suseconds_t = isize;

struct timeval
{
    time_t      tv_sec;  /* Seconds.  */
    suseconds_t tv_usec; /* Microseconds.  */
};

struct timezone
{
    i32 tz_minuteswest; /* minutes west of Greenwich */
    i32 tz_dsttime;     /* type of DST correction */
};

constexpr usize CLOCK_REALTIME  = 0;
constexpr usize CLOCK_MONOTONIC = 1;
