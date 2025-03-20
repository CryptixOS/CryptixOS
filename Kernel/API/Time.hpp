/*
 * Created by v1tr10l7 on 19.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

#include <cerrno>
#include <expected>

namespace Syscall::Time
{
    ErrorOr<i32> SysGetTimeOfDay(Syscall::Arguments& args);
    ErrorOr<i32> SysSetTimeOfDay(Syscall::Arguments& args);
} // namespace Syscall::Time

namespace API::Time
{
    ErrorOr<isize> SysClockGetTime(clockid_t id, timespec* res);
};
