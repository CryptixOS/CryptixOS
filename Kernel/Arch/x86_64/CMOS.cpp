/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/CMOS.hpp>
#include <Arch/x86_64/IO.hpp>

#include <utility>

namespace CMOS
{
    constexpr u8 REGISTER_SELECT = 0x70;
    constexpr u8 DATA            = 0x71;

    void         Write(Register reg, byte value)
    {
        IO::Out<byte>(REGISTER_SELECT, std::to_underlying(reg));
        IO::Wait();

        IO::Out<byte>(DATA, value);
    }
    byte Read(Register reg)
    {
        IO::Out<byte>(REGISTER_SELECT, std::to_underlying(reg));
        IO::Wait();

        return IO::In<byte>(DATA);
    }
} // namespace CMOS
