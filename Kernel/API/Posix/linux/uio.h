/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#ifndef CTOS__LINUX_UIO_H
#define CTOS__LINUX_UIO_H

#include <Prism/Core/Types.hpp>
#define __user

/*
 *	Berkeley style UIO structures	-	Alan Cox 1994.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
struct iovec
{
    /* BSD uses caddr_t (1003.1g requires void *) */
    void __user* iov_base;
    /* Must be usize (1003.1g) */
    usize        iov_len;
};

/*
 *	UIO_MAXIOV shall be at least 16 1003.1g (5.4.1.1)
 */
constexpr usize UIO_FASTIOV = 8;
constexpr usize UIO_MAXIOV  = 1024;

struct kvec
{
    /* and that should *never* hold a userland pointer */
    void* iov_base;
    usize iov_len;
};

/*
 * Total number of bytes covered by an iovec.
 *
 * NOTE that it is not safe to use this function until all the iovec's
 * segment lengths have been validated.  Because the individual lengths can
 * overflow a usize when added together.
 */
static inline usize iov_length(const struct iovec* iov, unsigned long nr_segs)
{
    unsigned long seg;
    usize         ret = 0;

    for (seg = 0; seg < nr_segs; seg++) ret += iov[seg].iov_len;
    return ret;
}

unsigned long iov_shorten(struct iovec* iov, unsigned long nr_segs, usize to);
#endif
