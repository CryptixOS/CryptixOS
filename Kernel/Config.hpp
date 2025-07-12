/*
 * Created by v1tr10l7 on 12.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

//--------------------------------------------------------------------------
// Compile-time kernel configuration
//--------------------------------------------------------------------------

#define KERNEL_VIRTUAL_RANGE_BASE 0xffff'0000'0000'0000zu
#define KERNEL_VIRTUAL_RANGE_TOP  0x6a9e'7f5c'8000'0000zu
#define KERNEL_VIRTUAL_RANGE_SIZE                                              \
    KERNEL_VIRTUAL_RANGE_TOP KERNEL_VIRTUAL_RANGE_BASE

#define USER_VIRTUAL_RANGE_BASE 0x0000'0000'0010'0000zu
#define USER_VIRTUAL_RANGE_TOP  0x0000'7fff'ffff'ffffzu
#define USER_VIRTUAL_RANGE_SIZE USER_VIRTUAL_RANGE_TOP - USER_VIRTUAL_RANGE_BASE
