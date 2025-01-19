/*
 * Created by v1tr10l7 on 15.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>

namespace Syscall::System
{
    std::expected<isize, std::errno_t> SysUname(Syscall::Arguments& args);
}
