/*
 * Created by v1tr10l7 on 05.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

typedef unsigned long int nfds_t;
struct pollfd
{
    int   fd;
    short events;
    short revents;
};
