/*
 * Created by v1tr10l7 on 05.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

/*
 * Desired design of maximum size and alignment (see RFC2553)
 */
/* Implementation specific max size */
constexpr usize        _K_SS_MAXSIZE = 128;

typedef unsigned short __kernel_sa_family_t;

/*
 * The definition uses anonymous union and struct in order to control the
 * default alignment.
 */
struct __kernel_sockaddr_storage
{
    union
    {
        struct
        {
            __kernel_sa_family_t ss_family; /* address family */
            /* Following field(s) are implementation specific */
            char                 __data[_K_SS_MAXSIZE - sizeof(unsigned short)];
            /* space to achieve desired size, */
            /* _SS_MAXSIZE value minus size of ss_family */
        };
        void* __align; /* implementation specific desired alignment */
    };
};

constexpr usize SOCK_SNDBUF_LOCK       = 1;
constexpr usize SOCK_RCVBUF_LOCK       = 2;

constexpr usize SOCK_BUF_LOCK_MASK     = (SOCK_SNDBUF_LOCK | SOCK_RCVBUF_LOCK);

constexpr usize SOCK_TXREHASH_DEFAULT  = 255;
constexpr usize SOCK_TXREHASH_DISABLED = 0;
constexpr usize SOCK_TXREHASH_ENABLED  = 1;
