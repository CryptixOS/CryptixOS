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
        for (usize slot = 0; slot < 32; slot++)
        {
            DeviceAddress address;
            address.Domain   = m_Domain;
            address.Bus      = m_Bus;
            address.Slot     = slot;
            address.Function = 0;

            u16 vendorID     = m_HostController->Read<u16>(
                address, ToUnderlying(RegisterOffset::eVendorID));
            if (vendorID == PCI_INVALID) continue;

            u8 headerType = m_HostController->Read<u8>(
                address, ToUnderlying(RegisterOffset::eHeaderType));
            EnumerateFunction(address);

            if ((headerType & 0x80) == 0) continue;
            for (u8 function = 1; function < 8; function++)
            {
                address.Function = function;
                if (m_HostController->Read<u16>(
                        address, ToUnderlying(RegisterOffset::eVendorID))
                    != PCI_INVALID)
                    EnumerateFunction(address);
            }
        }
    }

    Device* Bus::FindDeviceByID(const DeviceID& id)
    {
        for (const auto device : m_Devices)
            if (device->GetDeviceID() == id) return device;
        for (const auto& bridge : m_ChildBridges)
        {
            auto device = bridge->FindDeviceByID(id);
            if (device) return device;
        }

        return nullptr;
    }
    bool Bus::Enumerate(Enumerator enumerator)
    {
        for (const auto& device : m_Devices)
            if (enumerator(device->GetAddress())) return true;
        for (const auto& bridge : m_ChildBridges)
            if (enumerator(bridge->Address())) return true;
        return false;
    }

    void Bus::EnumerateFunction(DeviceAddress& address)
    {
        u8 classID = m_HostController->Read<u8>(
            address, ToUnderlying(RegisterOffset::eClassID));
        u8 subClassID = m_HostController->Read<u8>(
            address, ToUnderlying(RegisterOffset::eSubClassID));
        if (classID == 0x06 && subClassID == 0x04)
            LogTrace(
                "PCI: Discovered PCI-To-PCI Bridge => "
                "{:#04x}.{:#04x}.{:#04x}.{:#04x}",
                address.Domain, address.Bus, address.Slot, address.Function);

        u16 vendorID = m_HostController->Read<u16>(
            address, ToUnderlying(RegisterOffset::eVendorID));
        if (vendorID == PCI_INVALID) return;

        u16 deviceID = m_HostController->Read<u16>(
            address, ToUnderlying(RegisterOffset::eDeviceID));

        u8 headerType = m_HostController->Read<u8>(
                            address, ToUnderlying(RegisterOffset::eHeaderType))
                      & 0x7f;

        if (headerType == 0x00)
        {
            auto device = new Device(address);
            LogInfo(
                "PCI: Device discovered => {:#04x}:{:#04x}:{:#04x}:{:#04x}, "
                "with class {:#04x}:{:#04x}",
                address.Domain, address.Bus, address.Slot, address.Function,
                classID, subClassID);

            m_Devices.PushBack(device);
        }
        else if (headerType == 0x01)
        {
            u8 secondaryBus = m_HostController->Read<u8>(
                address, ToUnderlying(RegisterOffset::eSecondaryBus));
            u8 subordinateBus = m_HostController->Read<u8>(
                address, ToUnderlying(RegisterOffset::eSubordinateBus));
            LogTrace(
                "PCI: Found PCI-To-PCI Bridge: {:#04x}:{:#04x} => "
                "{:#04x}:{:#04x}",
                vendorID, deviceID, secondaryBus, subordinateBus);

            for (u8 bus = secondaryBus; bus <= subordinateBus; ++bus)
            {
                if (m_HostController->Read<u16>(
                        {0, 0, bus}, ToUnderlying(RegisterOffset::eVendorID))
                    == PCI_INVALID)
                    continue;

                auto bridge = new Bus(m_HostController,
                                      m_HostController->GetDomain().ID, bus);
                m_ChildBridges.PushBack(bridge);
                bridge->DetectDevices();
            }
        }
        else
            LogTrace("PCI: Encountered CardBus Bridge: {:#04x}:{:#04x}",
                     vendorID, deviceID);

        auto command = m_HostController->Read<u16>(
            address, ToUnderlying(RegisterOffset::eCommand));
        m_HostController->Write<u16>(
            address, ToUnderlying(RegisterOffset::eCommand), command | 0x400);
    }
} // namespace PCI
