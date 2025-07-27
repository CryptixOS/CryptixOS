/*
 * Created by v1tr10l7 on 19.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

namespace API::Time
{
    ErrorOr<isize> GetTimeOfDay(struct timeval* restrict tv,
                                struct timezone* tz);
    ErrorOr<isize> SetTimeOfDay(const struct timeval* restrict tv,
                                const struct timezone* tz);
    ErrorOr<isize> ClockGetTime(clockid_t id, timespec* res);
}; // namespace API::Time
