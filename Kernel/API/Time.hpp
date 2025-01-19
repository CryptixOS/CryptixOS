/*
 * Created by v1tr10l7 on 19.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>

#include <cerrno>
#include <expected>

namespace Syscall::Time
{
    std::expected<i32, std::errno_t> SysGetTimeOfDay(Syscall::Arguments& args);
    std::expected<i32, std::errno_t> SysSetTimeOfDay(Syscall::Arguments& args);
} // namespace Syscall::Time
