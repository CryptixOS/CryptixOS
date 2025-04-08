/*
 * Created by v1tr10l7 on 01.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/time.h>
#include <Prism/Core/Types.hpp>

using rlim_t = u64;

struct rlimit
{
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

struct rusage
{
    timeval ru_utime;
    timeval ru_stime;
    i64     ru_maxrss;
    i64     ru_ixrss;
    i64     ru_idrss;
    i64     ru_isrss;
    i64     ru_minflt;
    i64     ru_majflt;
    i64     ru_nswap;
    i64     ru_inblock;
    i64     ru_oublock;
    i64     ru_msgsnd;
    i64     ru_msgrscv;
    i64     ru_nsignals;
    i64     ru_nvcsw;
    i64     ru_nivcsw;
};

constexpr usize RLIMIT_CPU        = 0;
constexpr usize RLIMIT_FSIZE      = 1;
constexpr usize RLIMIT_DATA       = 2;
constexpr usize RLIMIT_STACK      = 3;
constexpr usize RLIMIT_CORE       = 4;

constexpr usize RLIMIT_RSS        = 5;
constexpr usize RLIMIT_NPROC      = 6;
constexpr usize RLIMIT_NOFILE     = 7;
constexpr usize RLIMIT_MEMLOCK    = 8;
constexpr usize RLIMIT_AS         = 9;
constexpr usize RLIMIT_LOCKS      = 10;
constexpr usize RLIMIT_SIGPENDING = 11;
constexpr usize RLIMIT_MSGQUEUE   = 12;
constexpr usize RLIMIT_NICE       = 13;

constexpr usize RLIMIT_RTPRIO     = 14;
constexpr usize RLIMIT_RTTIME     = 15;
constexpr usize RLIM_NLIMITS      = 16;

constexpr usize RLIM_INFINITY     = usize(~0ul);
