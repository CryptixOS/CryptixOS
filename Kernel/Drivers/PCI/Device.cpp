/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/HostController.hpp>
#include <Drivers/PCI/PCI.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Math.hpp>

namespace PCI
{
    void Device::EnableMemorySpace()
    {
        ScopedLock guard(m_Lock);

        u16        cmd = Read<u16>(RegisterOffset::eCommand) | Bit(0);
        Write<u16>(RegisterOffset::eCommand, cmd);
    }
    void Device::EnableBusMastering()
    {
        ScopedLock guard(m_Lock);
        u16        value = Read<u16>(RegisterOffset::eCommand);
        value |= Bit(2);
        value |= Bit(0);

        Write<u16>(RegisterOffset::eCommand, value);
    }

    Bar Device::GetBar(u8 index)
    {
        Bar bar = {0};

        if (index > 5) return bar;

        u16 offset  = 0x10 + index * sizeof(u32);
        u32 baseLow = ReadAt(offset, 4);
        WriteAt(offset, ~0, 4);
        u32 sizeLow = ReadAt(offset, 4);
        WriteAt(offset, baseLow, 4);

        if (baseLow & 1)
        {
            bar.Address = baseLow & ~0b11;
            bar.Size    = ~(sizeLow & ~0b11) + 1;
        }
        else
        {
            i32 type     = (baseLow >> 1) & 3;
            u32 baseHigh = ReadAt(offset + 4, 4);

            bar.Address  = baseLow & 0xfffffff0;
            if (type == 2) bar.Address |= ((u64)baseHigh << 32);

            bar.Size   = ~(sizeLow & ~0b1111) + 1;
            bar.IsMMIO = true;
        }

        return bar;
    }

    u32 Device::ReadAt(u32 offset, i32 accessSize) const
    {
        auto* controller = GetHostController(m_Address.Domain);
        return controller->Read(m_Address, offset, accessSize);
    }
    void Device::WriteAt(u32 offset, u32 value, i32 accessSize) const
    {
        auto* controller = GetHostController(m_Address.Domain);
        controller->Write(m_Address, offset, value, accessSize);
    }
}; // namespace PCI
