/*
 * Created by v1tr10l7 on 16.05.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/mount.h>

enum class MountFlags : u8
{
    eNone     = 0,
    eReadOnly = MS_RDONLY,
};
