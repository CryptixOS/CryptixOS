/*
 * Created by v1tr10l7 on 14.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

using clockid_t              = i32;
using time_t                 = i64;
using suseconds_t            = isize;

constexpr usize NSEC_PER_SEC = 1000000000l;
constexpr usize USEC_PER_SEC = 1000000l;

struct timespec
{
    time_t                           tv_sec;
    time_t                           tv_nsec;

    inline static constexpr timespec FromNanoseconds(usize ns)
    {
        if (ns > 0) [[likely]]
            return timespec(ns / NSEC_PER_SEC, ns % NSEC_PER_SEC);
        if (!ns) return timespec(0, 0);

        timespec ts;
        ts.tv_sec  = ((-ns - 1) / NSEC_PER_SEC) * -1 - 1;
        ts.tv_nsec = NSEC_PER_SEC - ((-ns - 1) % NSEC_PER_SEC) - 1;
        return ts;
    }
    inline constexpr operator bool() const
    {
        return tv_sec != 0 || tv_nsec != 0;
    }
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
using timespec64 = timespec;

struct timeval
{
    /* seconds */
    time_t           tv_sec;
    /* microseconds */
    suseconds_t      tv_usec;

    inline constexpr operator bool() const
    {
        return tv_sec >= 0 && static_cast<usize>(tv_usec) < USEC_PER_SEC;
    }
};

struct itimerspec
{
    /* timer period */
    struct timespec it_interval;
    /* timer expiration */
    struct timespec it_value;
};

struct itimerval
{
    /* timer interval */
    struct timeval it_interval;
    /* current value */
    struct timeval it_value;
};

struct timezone
{
    /* minutes west of Greenwich */
    int tz_minuteswest;
    /* type of dst correction */
    int tz_dsttime;
};

/*
 * Names of the interval timers, and structure
 * defining a timer setting:
 */
constexpr usize ITIMER_REAL              = 0;
constexpr usize ITIMER_VIRTUAL           = 1;
constexpr usize ITIMER_PROF              = 2;

/*
 * The IDs of the various system clocks (for POSIX.1b interval timers):
 */
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
/*
 * The driver implementing this got removed. The clock ID is kept as a
 * place holder. Do not reuse!
 */
constexpr usize CLOCK_SGI_CYCLE          = 10;
constexpr usize CLOCK_TAI                = 11;

constexpr usize MAX_CLOCKS               = 16;
constexpr usize CLOCKS_MASK              = (CLOCK_REALTIME | CLOCK_MONOTONIC);
constexpr usize CLOCKS_MONO              = CLOCK_MONOTONIC;

/*
 * The various flags for setting POSIX.1b interval timers:
 */
constexpr usize TIMER_ABSTIME            = 0x01;
