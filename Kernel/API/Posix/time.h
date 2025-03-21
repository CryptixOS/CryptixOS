/*
 * Created by v1tr10l7 on 20.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

using clockid_t   = i32;
using time_t      = i64;
using suseconds_t = isize;

struct timespec
{
    time_t           tv_sec;
    time_t           tv_nsec;

    constexpr auto   operator<=>(const timespec& other) const = default;
    inline timespec& operator-=(const timespec& other)
    {
        if (other.tv_nsec > tv_nsec)
        {
            tv_nsec = 999999999 - (other.tv_nsec - tv_nsec);
            if (tv_sec == 0)
            {
                tv_sec = tv_nsec = 0;
                return *this;
            }
            tv_sec--;
        }
        else tv_nsec -= other.tv_nsec;

        if (other.tv_sec > tv_sec)
        {
            tv_sec = tv_nsec = 0;
            return *this;
        }
        tv_sec -= other.tv_sec;

        return *this;
    }
};

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

/* Identifier for system-wide realtime clock.  */
constexpr usize CLOCK_REALTIME           = 0;
/* Monotonic system-wide clock.  */
constexpr usize CLOCK_MONOTONIC          = 1;
/* High-resolution timer from the CPU.  */
constexpr usize CLOCK_PROCESS_CPUTIME_ID = 2;
/* Thread-specific CPU-time clock.  */
constexpr usize CLOCK_THREAD_CPUTIME_ID  = 3;
/* Monotonic system-wide clock, not adjusted for frequency scaling.  */
constexpr usize CLOCK_MONOTONIC_RAW      = 4;
/* Identifier for system-wide realtime clock, updated only on ticks.  */
constexpr usize CLOCK_REALTIME_COARSE    = 5;
/* Monotonic system-wide clock, updated only on ticks.  */
constexpr usize CLOCK_MONOTONIC_COARSE   = 6;
/* Monotonic system-wide clock that includes time spent in suspension.  */
constexpr usize CLOCK_BOOTTIME           = 7;
/* Like CLOCK_REALTIME but also wakes suspended system.  */
constexpr usize CLOCK_REALTIME_ALARM     = 8;
/* Like CLOCK_BOOTTIME but also wakes suspended system.  */
constexpr usize CLOCK_BOOTTIME_ALARM     = 9;
