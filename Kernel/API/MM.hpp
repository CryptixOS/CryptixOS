/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "API/Syscall.hpp"
#include "Common.hpp"

constexpr uintptr_t MAP_FAILED    = -1;
constexpr usize     MAP_ANONYMOUS = 0x20;

namespace Syscall::MM
{
    uintptr_t SysMMap(Syscall::Arguments& args);
} // namespace Syscall::MM
