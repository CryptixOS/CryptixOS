/*
 * Created by v1tr10l7 on 22.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>

namespace Syscall
{
    ErrorOr<isize> SysPipe(Arguments& args);
    ErrorOr<isize> SysNanoSleep(Arguments& args);
    ErrorOr<isize> SysCreat(Arguments& args);
    ErrorOr<isize> SysReadLink(Arguments& args);
    ErrorOr<isize> SysClockGetTime(Arguments& args);
}; // namespace Syscall
