/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/PCI/Access.hpp>

#include <Utility/Types.hpp>

namespace PCI
{

    u64 MemoryIoSpace::Read(const DeviceAddress& address, RegisterOffset reg,
                            u8 byteWidth)
    {
        uintptr_t offset     = static_cast<uintptr_t>(reg);
        Pointer   regAddress = m_Address.Offset(offset);

        switch (byteWidth)
        {
            case 1: return *regAddress.As<u8>();
            case 2: return *regAddress.As<u16>();
            case 4: return *regAddress.As<u32>();

            default: break;
        }

        assert(false);
        return -1;
    }
    void MemoryIoSpace::Write(const DeviceAddress& address, RegisterOffset reg,
                              u64 value, u8 byteWidth)
    {
        uintptr_t offset     = static_cast<uintptr_t>(reg);
        Pointer   regAddress = m_Address.Offset(offset);

        switch (byteWidth)
        {
            case 1: *regAddress.As<u8>() = value; break;
            case 2: *regAddress.As<u16>() = value; break;
            case 4: *regAddress.As<u32>() = value; break;

            default: assert(false); break;
        }
    }
} // namespace PCI
