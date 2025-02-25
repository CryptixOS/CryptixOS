/*
 * Created by v1tr10l7 on 15.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>

constexpr usize _UTSNAME_SYSNAME_LENGTH = 65;

struct utsname
{
    char sysname[_UTSNAME_SYSNAME_LENGTH];
    char nodename[_UTSNAME_SYSNAME_LENGTH];
    char release[_UTSNAME_SYSNAME_LENGTH];
    char version[_UTSNAME_SYSNAME_LENGTH];
    char machine[_UTSNAME_SYSNAME_LENGTH];
};
