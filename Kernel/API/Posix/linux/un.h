/*
 * Created by v1tr10l7 on 05.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <linux/socket.h>

#define UNIX_PATH_MAX 108

struct sockaddr_un
{
    __kernel_sa_family_t sun_family;              /* AF_UNIX */
    char                 sun_path[UNIX_PATH_MAX]; /* pathname */
};

#define SIOCUNIXFILE                                                           \
    (SIOCPROTOPRIVATE + 0) /* open a socket file with O_PATH                   \
                            */
