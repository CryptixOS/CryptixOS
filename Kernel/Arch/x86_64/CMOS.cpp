/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Prism/Core/Concepts.hpp>
#include <Prism/Core/Core.hpp>

module Cryptix:Impl.Arch.x86_64.CMOS;

import :Arch.x86_64.CMOS;
import :Arch.x86_64.IO;

namespace CMOS
{
    constexpr u8 REGISTER_SELECT = 0x70;
    constexpr u8 DATA            = 0x71;

    void         Write(Register reg, byte value)
    {
        IO::Out<byte>(REGISTER_SELECT, ToUnderlying(reg));
        IO::Wait();

        IO::Out<byte>(DATA, value);
    }
    byte Read(Register reg)
    {
        IO::Out<byte>(REGISTER_SELECT, ToUnderlying(reg));
        IO::Wait();

        return IO::In<byte>(DATA);
    }
} // namespace CMOS
