/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <errno.h>

static errno_t      error;

extern "C" errno_t* __errno_location()
{
    return reinterpret_cast<errno_t*>(&error);
}
