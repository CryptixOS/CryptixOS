/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/CMOS.hpp>

#include <Arch/x86_64/IO.hpp>

namespace CMOS
{
    constexpr u8 CMOS      = 0x70;
    constexpr u8 CMOS_DATA = 0x71;

    void         Write(byte reg, byte value)
    {
        IO::Out<byte>(CMOS, reg);
        IO::Wait();

        IO::Out<byte>(CMOS_DATA, value);
    }
    byte Read(byte reg)
    {
        IO::Out<byte>(CMOS, reg);
        IO::Wait();

        return IO::In<byte>(CMOS_DATA);
    }
} // namespace CMOS
