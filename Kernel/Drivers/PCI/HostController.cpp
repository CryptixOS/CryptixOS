/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/IO.hpp>
#include <Drivers/PCI/HostController.hpp>

#include <Memory/MMIO.hpp>
#include <Memory/VMM.hpp>
#include <Library/Logger.hpp>

namespace PCI
{
    bool HostController::EnumerateDevices(Enumerator enumerator)
    {
        return EnumerateRootBus(enumerator);
    }

    bool HostController::EnumerateRootBus(Enumerator enumerator)
    {
        if ((Read<u8>(DeviceAddress(),
                      std::to_underlying(RegisterOffset::eHeaderType))
             & Bit(7))
            == 0)
        {
            EnumerateBus(0, enumerator);
            return false;
        }
        else
        {
            for (u8 i = 0; i < 8; ++i)
            {
                if (Read<u16>({0, 0, i},
                              std::to_underlying(RegisterOffset::eVendorID))
                    == PCI_INVALID)
                    continue;
                if (EnumerateBus(i, enumerator)) return true;
            }
        }

        return false;
    }
    bool HostController::EnumerateBus(u8 bus, Enumerator enumerator)
    {
        for (u8 slot = 0; slot < 32; ++slot)
            if (EnumerateSlot(bus, slot, enumerator)) return true;

        return false;
    }
    bool HostController::EnumerateSlot(const u8 bus, const u8 slot,
                                       Enumerator enumerator)
    {
        DeviceAddress address{};
        address.Domain   = m_Domain.ID;
        address.Bus      = bus;
        address.Slot     = slot;
        address.Function = 0;

        u16 vendorID
            = Read<u16>(address, std::to_underlying(RegisterOffset::eVendorID));
        if (vendorID == PCI_INVALID) return false;

        u8 headerType = Read<u8>(
            address, std::to_underlying(RegisterOffset::eHeaderType));
        if (!(headerType & Bit(7)))
        {
            if (EnumerateFunction(address, enumerator)) return true;
        }
        else
        {
            for (u8 i = 0; i < 8; ++i)
            {
                address.Function = i;
                if (Read<u16>(address,
                              std::to_underlying(RegisterOffset::eVendorID))
                    != PCI_INVALID)
                {
                    if (EnumerateFunction(address, enumerator)) return true;
                }
            }
        }

        return false;
    }
    bool HostController::EnumerateFunction(const DeviceAddress& address,
                                           Enumerator           enumerator)
    {
        if (Read<u8>(address, std::to_underlying(RegisterOffset::eHeaderType))
            == 0)
        {
            LogDebug(
                "Interrupt Line: {:#x}, Interrupt Pin: {:#x}",
                Read<u8>(address,
                         std::to_underlying(RegisterOffset::eInterruptLine)),
                Read<u8>(address,
                         std::to_underlying(RegisterOffset::eInterruptPin)));
        }

        if (enumerator(address)) return true;
        u16 type
            = Read<u8>(address, std::to_underlying(RegisterOffset::eClassID))
           << 8;
        type |= Read<u8>(address,
                         std::to_underlying(RegisterOffset::eSubClassID));

        if (type == ((0x06 << 8) | 0x04))
        {
            u8 secondaryBus = Read<u8>(
                address, std::to_underlying(RegisterOffset::eSecondaryBus));
            if (EnumerateBus(secondaryBus, enumerator)) return true;
        }
        return false;
    }
}; // namespace PCI
