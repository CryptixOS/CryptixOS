/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Types.hpp"

using cc_t           = u32;
using speed_t        = u32;
using tcflag_t       = u32;

constexpr usize NCCS = 11;

struct termios
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t     c_cc[NCCS];
    speed_t  ibaud;
    speed_t  obaud;
};
