/*
 * Created by v1tr10l7 on 25.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/PCI/Bus.hpp>
#include <Drivers/PCI/HostController.hpp>

namespace PCI
{
    void Bus::DetectDevices()
    {
        for (usize slot = 0; slot < 32; ++slot)
        {
            DeviceAddress address;
            address.Domain   = m_Domain;
            address.Bus      = m_Bus;
            address.Slot     = slot;
            address.Function = 0;

            u16 vendorID     = m_HostController->Read<u16>(
                address, std::to_underlying(RegisterOffset::eVendorID));
            if (vendorID == PCI_INVALID) return;

            u8 headerType = m_HostController->Read<u8>(
                address, std::to_underlying(RegisterOffset::eHeaderType));
            if (!(headerType & Bit(7))) EnumerateFunction(address);
            else
            {
                for (u8 i = 0; i < 8; ++i)
                {
                    address.Function = i;
                    if (m_HostController->Read<u16>(
                            address,
                            std::to_underlying(RegisterOffset::eVendorID))
                        != PCI_INVALID)
                        EnumerateFunction(address);
                }
            }
        }
    }

    bool Bus::Enumerate(Enumerator enumerator)
    {
        for (const auto& device : m_Devices)
            if (enumerator(device->GetAddress())) return true;
        return false;
    }

    void Bus::EnumerateFunction(DeviceAddress& address)
    {
        u16 vendorID = m_HostController->Read<u16>(
            address, std::to_underlying(RegisterOffset::eVendorID));
        u16 deviceID = m_HostController->Read<u16>(
            address, std::to_underlying(RegisterOffset::eDeviceID));
        if (vendorID == PCI_INVALID || deviceID == PCI_INVALID) return;

        u8 headerType
            = m_HostController->Read<u8>(
                  address, std::to_underlying(RegisterOffset::eHeaderType))
            & 0x7f;

        if (headerType == 0x00)
        {
            auto device = new Device(address);

            m_Devices.PushBack(device);
        }
        else if (headerType == 0x01)
        {
            [[maybe_unused]] u8 secondaryBus = m_HostController->Read<u8>(
                address, std::to_underlying(RegisterOffset::eSecondaryBus));

            LogTrace("PCI: Found PCI-To-PCI Bridge: {:#04x}:{:#04x}", vendorID,
                     deviceID);
        }
    }
} // namespace PCI
