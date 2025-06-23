/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

constexpr isize FD_SETSIZE = 1024;
struct fd_set
{
    u8 fds_bits[128];
};

inline constexpr void FD_CLR(isize fdNum, fd_set* set)
{
    assert(fdNum < FD_SETSIZE);
    set->fds_bits[fdNum / 8] &= ~(1 << (fdNum % 8));
}
inline constexpr isize FD_ISSET(isize fdNum, fd_set* set)
{
    assert(fdNum < FD_SETSIZE);
    return set->fds_bits[fdNum / 8] & (1 << (fdNum % 8));
}
inline constexpr void FD_SET(isize fdNum, fd_set* set)
{
    assert(fdNum < FD_SETSIZE);
    set->fds_bits[fdNum / 8] |= 1 << (fdNum % 8);
}
inline constexpr void FD_ZERO(fd_set* set)
{
    std::memset(set->fds_bits, 0, sizeof(fd_set));
}
