/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/HostController.hpp>
#include <Drivers/PCI/PCI.hpp>
#include <Drivers/Storage/NVMe/NVMeController.hpp>

#include <Firmware/ACPI/MCFG.hpp>
#include <Library/Logger.hpp>

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
    static bool EnumerateDevices(Enumerator enumerator)
    {
        for (auto& [domain, controller] : s_HostControllers)
            if (controller->EnumerateDevices(enumerator)) return true;
        return false;
    }

    void Initialize()
    {
        DetectControllers();
        if (s_HostControllers.empty())
        {
            Domain domain(0, 0, 32);
            s_HostControllers[0] = new HostController(domain, 0);
        }

        Enumerator enumerator;
        enumerator.BindLambda(
            [](const DeviceAddress& addr) -> bool
            {
                HostController* controller = s_HostControllers[addr.Domain];
                VendorID vendorID = static_cast<VendorID>(controller->Read<u16>(
                    addr, std::to_underlying(RegisterOffset::eVendorID)));
                u16      deviceID = controller->Read<u16>(
                    addr, std::to_underlying(RegisterOffset::eDeviceID));
                u8 classID = controller->Read<u8>(
                    addr, std::to_underlying(RegisterOffset::eClassID));
                u8 subclassID = controller->Read<u8>(
                    addr, std::to_underlying(RegisterOffset::eSubClassID));

                std::string_view vendorName = magic_enum::enum_name(vendorID);
                if (vendorName.empty()) vendorName = "Unrecognized";

                LogInfo(
                    "PCI: {:#x}:{:#x}:{:#x}, ID: {:#x}:{:#x}:{:#x}:{:#x} - {}",
                    addr.Bus, addr.Slot, addr.Function,
                    static_cast<u16>(vendorID), deviceID, classID, subclassID,
                    vendorName);

                if (s_HostControllers[addr.Domain]->Read<u8>(
                        addr, std::to_underlying(RegisterOffset::eClassID))
                        == 0x01
                    && s_HostControllers[addr.Domain]->Read<u8>(
                           addr,
                           std::to_underlying(RegisterOffset::eSubClassID))
                           == 0x08)
                {
                    LogInfo(
                        "NVMe{}: {{ Domain: {}, Bus: {}, Slot: {}, Function: "
                        "{} }}",
                        0, addr.Domain, addr.Bus, addr.Slot, addr.Function);
                    new NVMe::Controller(addr);
                }
                return false;
            });

        EnumerateDevices(enumerator);
    }

    HostController* GetHostController(u32 domain)
    {
        Assert(s_HostControllers.contains(domain));
        return s_HostControllers[domain];
    }
    bool RegisterDriver(struct Driver& driver)
    {
        DeviceID      foundID;
        DeviceAddress foundAddress;
        bool          found = false;

        Enumerator    enumerator;
        enumerator.BindLambda(
            [&](const DeviceAddress& addr) -> bool
            {
                PCI::Device device(addr);

                if (device.MatchID(driver.MatchIDs, foundID))
                {
                    foundAddress = addr;
                    found        = true;
                    return true;
                }

                return false;
            });

        if (EnumerateDevices(enumerator))
        {
            Device device(foundAddress);
            driver.Probe(foundAddress, foundID);
            return true;
        }

        return false;
    }
}; // namespace PCI
