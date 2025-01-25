/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/IO.hpp>
#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/HostController.hpp>
#include <Drivers/PCI/PCI.hpp>

#include <Firmware/ACPI/MCFG.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <cctype>
#include <magic_enum/magic_enum.hpp>
#include <unordered_map>

namespace PCI
{
    static std::unordered_map<u32, HostController*> s_HostControllers;

    static void                                     DetectControllers()
    {
        const MCFG* mcfg       = ACPI::GetTable<MCFG>(MCFG_SIGNATURE);
        usize       entryCount = 0;

        if (!mcfg)
        {
            LogWarn(
                "PCI: MCFG table not found, falling back to legacy "
                "io....");
            return;
        }

        if (mcfg->Header.Length < sizeof(MCFG) + sizeof(MCFG::Entry))
        {
            LogWarn("PCI: MCFG table found, but with no entries");
            return;
        }

        entryCount = (mcfg->Header.Length - sizeof(MCFG)) / sizeof(MCFG::Entry);
        for (usize i = 0; i < entryCount; i++)
        {
            const MCFG::Entry& entry    = mcfg->Entries[i];
            u64                address  = entry.Address;
            u8                 busStart = entry.StartBus;
            u8                 busEnd   = entry.EndBus;

            LogInfo("PCI: Discovered domain at '{:#x}', Buses ({} - {})",
                    address, busStart, busEnd);
            Domain domain(i, busStart, busEnd);
            s_HostControllers[domain.ID] = new HostController(domain, address);
        }
    }
    static void EnumerateDevices(Enumerator enumerator)
    {
        for (auto& [domain, controller] : s_HostControllers)
            if (controller->EnumerateDevices(enumerator)) break;
    }

    void Initialize()
    {
        DetectControllers();
        if (s_HostControllers.empty())
        {
            Domain domain(0, 0, 32);
            s_HostControllers[0] = new HostController(domain, 0);
        }

        EnumerateDevices(
            [](const DeviceAddress& addr)
            {
                HostController* controller = s_HostControllers[addr.Domain];
                VendorID vendorID = static_cast<VendorID>(controller->Read<u16>(
                    addr, std::to_underlying(RegisterOffset::eVendorID)));
                u16      deviceID = controller->Read<u16>(
                    addr, std::to_underlying(RegisterOffset::eDeviceID));

                std::string_view vendorName = magic_enum::enum_name(vendorID);
                if (vendorName.empty()) vendorName = "Unrecognized";

                LogInfo("PCI: {:#x}:{:#x}:{:#x}, ID: {:#x}:{:#x} - {}",
                        addr.Bus, addr.Slot, addr.Function,
                        static_cast<u16>(vendorID), deviceID, vendorName);

                if (s_HostControllers[addr.Domain]->Read<u8>(
                        addr, std::to_underlying(RegisterOffset::eClassID))
                        == 0x01
                    && s_HostControllers[addr.Domain]->Read<u8>(
                           addr,
                           std::to_underlying(RegisterOffset::eSubClassID))
                           == 0x08)
                {
                    // TODO(v1tr10l7): Initialize the NVMeController
                }
                return false;
            });
    }

    HostController* GetHostController(u32 domain)
    {
        Assert(s_HostControllers.contains(domain));
        return s_HostControllers[domain];
    }
}; // namespace PCI
