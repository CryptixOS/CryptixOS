/*
 * Created by v1tr10l7 on 25.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/IO.hpp>

#include <Drivers/PCI/Access.hpp>
#include <Drivers/PCI/Device.hpp>

namespace PCI
{
    constexpr usize CONFIG_ADDRESS = 0x0cf8;
    constexpr usize CONFIG_DATA    = 0x0cfc;
    u32 LegacyAccessMechanism::Read(const DeviceAddress& dev, u32 offset,
                                    i32 accessSize)
    {
        u32 address = Bit(31) | (offset & ~0x3) | (dev.Function << 8)
                    | (dev.Slot << 11) | (dev.Bus << 16);
        IO::Out<u32>(CONFIG_ADDRESS, address);

        u32 data = IO::In<u32>(CONFIG_DATA);
        data >>= (offset & 0x3) << 3;

        switch (accessSize)
        {
            case 1: return static_cast<u8>(data);
            case 2: return static_cast<u16>(data);
            case 4: return static_cast<u32>(data);

            default: break;
        }

        LogWarn(
            "PCI::LegacyAccessMechanism::Read: Called with invalid accessSize: "
            "'{}'!",
            accessSize);
        return 0;
    }
    void LegacyAccessMechanism::Write(const DeviceAddress& dev, u32 offset,
                                      u32 value, i32 accessSize)
    {
        u32 address = Bit(31) | (offset & ~3) | (dev.Function << 8)
                    | (dev.Slot << 11) | (dev.Bus << 16);
        IO::Out<u32>(CONFIG_ADDRESS, address);
        u32 oldVal = IO::In<u32>(CONFIG_DATA);
        u32 mask   = 0;

        switch (accessSize)
        {
            case 1: mask = 0xff; break;
            case 2: mask = 0xffff; break;
            case 4: mask = 0xffffffff; break;

            default: break;
        }

        i32 bitOffset = (offset & 0x3) << 3;
        value         = (value & mask) << bitOffset;

        oldVal &= ~(mask << bitOffset);
        oldVal |= value;

        IO::Out<u32>(CONFIG_ADDRESS, address);
        IO::Out<u32>(CONFIG_ADDRESS, oldVal);
    }
}; // namespace PCI
