/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

/* VESA Blanking Levels */
enum vesa_blank_mode
{
    VESA_NO_BLANKING = 0,
#define VESA_NO_BLANKING VESA_NO_BLANKING
    VESA_VSYNC_SUSPEND = 1,
#define VESA_VSYNC_SUSPEND VESA_VSYNC_SUSPEND
    VESA_HSYNC_SUSPEND = 2,
#define VESA_HSYNC_SUSPEND VESA_HSYNC_SUSPEND
    VESA_POWERDOWN = VESA_VSYNC_SUSPEND | VESA_HSYNC_SUSPEND,
#define VESA_POWERDOWN VESA_POWERDOWN
    VESA_BLANK_MAX = VESA_POWERDOWN,
};
