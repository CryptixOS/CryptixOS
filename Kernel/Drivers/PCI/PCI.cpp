/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
#include <Prism/Memory/Scope.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Prism/String/StringUtils.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/Posix/sys/statfs.h>
#include <cctype>

module Cryptix:Impl.Drivers.PCI.PCI;

import :Drivers.PCI.Device;
import :Drivers.PCI.HostController;
import :Drivers.PCI.PCI;
import :Drivers.Storage.NVMe.NVMeController;
import :Firmware.ACPI.MCFG;
import :Library.Logger;
import :VFS.INode;
import :VFS.VFS;

namespace PCI
{
    static UnorderedMap<u32, HostController*> s_HostControllers;
    struct Vendor
    {
        String                    Name;
        UnorderedMap<u16, String> DeviceIDs;
    };

    UnorderedMap<u16, Vendor> s_VendorIDs;

    void                      ParseVendorID(StringView line)
    {
        using namespace Prism::CodePoints;

        if (line.Size() < 4) return;
        for (usize i = 0; i < 4; i++)
            if (!IsHexDigit(line[i])) return;

        usize nameStartPos = 4;
        while (nameStartPos < line.Size() && !IsAlpha(line[nameStartPos]))
            ++nameStartPos;
        if (nameStartPos >= line.Size()) return;

        u16        id       = StringUtils::ToNumber<u16>(line.Substr(0, 4), 16);
        auto&      vendor   = s_VendorIDs[id];
        usize      nameSize = line.Size() - nameStartPos;
        StringView name(line.Raw() + nameStartPos, nameSize);
        if (line.Empty()) return;

        vendor.Name     = name;
        s_VendorIDs[id] = vendor;
    }
    void InitializeDatabase()
    {
        PathView path  = "/usr/share/hwdata/pci.ids";
        Ref      entry = VFS::ResolvePath(VFS::RootDirectoryEntry().Raw(), path)
                        .value()
                        .Entry;
        if (!entry) return;

        auto file = entry->INode();

        if (!file)
        {
            LogError("PCI: Failed to open pciids database");
            return;
        }

        usize fileSize = file->Stats().st_size;
        assert(fileSize > 0);

        String buffer;
        buffer.Resize(fileSize);
        fileSize         = file->Read(buffer.Raw(), 0, fileSize);

        usize currentPos = 0;
        usize newLinePos = String::NPos;
        while ((newLinePos = buffer.FindFirstOf("\n", currentPos))
                   < buffer.Size()
               && currentPos < buffer.Size())
        {
            usize  lineLength = newLinePos - currentPos;
            String line       = buffer.Substr(currentPos, lineLength);
            currentPos        = buffer.FindFirstNotOf("\n\r", newLinePos);
            if (line.Empty()) continue;

            ParseVendorID(line);
        }
    }

    static void DetectControllers()
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
            s_HostControllers[domain.ID]->Initialize();
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
        if (s_HostControllers.Empty())
        {
            Domain domain(0, 0, 32);
            s_HostControllers[0] = new HostController(domain, 0);
            s_HostControllers[0]->Initialize();
        }

        Enumerator enumerator;
        enumerator.BindLambda(
            [](const DeviceAddress& addr) -> bool
            {
                HostController* controller = s_HostControllers[addr.Domain];
                u16             vendorID   = controller->Read<u16>(
                    addr, ToUnderlying(RegisterOffset::eVendorID));
                u16 deviceID = controller->Read<u16>(
                    addr, ToUnderlying(RegisterOffset::eDeviceID));
                u8 classID = controller->Read<u8>(
                    addr, ToUnderlying(RegisterOffset::eClassID));
                u8 subClassID = controller->Read<u8>(
                    addr, ToUnderlying(RegisterOffset::eSubClassID));

                auto       vendor     = s_VendorIDs.Find(vendorID);
                StringView vendorName = vendor != s_VendorIDs.end()
                                          ? vendor->Value.Name.View()
                                          : "Unrecognized"_sv;

                LogInfo(
                    "PCI: {:#x}:{:#x}:{:#x}, ID: {:#x}:{:#x}:{:#x}:{:#x} - {}",
                    addr.Bus, addr.Slot, addr.Function,
                    static_cast<u16>(vendorID), deviceID, classID, subClassID,
                    vendorName);

                if (classID == 0x01 && subClassID == 0x08)
                {
                    LogInfo(
                        "NVMe{}: {{ Domain: {}, Bus: {}, Slot: {}, Function: "
                        "{} }}",
                        0, addr.Domain, addr.Bus, addr.Slot, addr.Function);
                    new NVMe::Controller(addr);
                }
                return false;
            });

        InitializeDatabase();
        EnumerateDevices(enumerator);
    }

    void InitializeIrqRoutes()
    {
        for (const auto& [domain, controller] : s_HostControllers)
            controller->InitializeIrqRoutes();
    }

    HostController* GetHostController(u32 domain)
    {
        Assert(s_HostControllers.Contains(domain));
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

    Device* FindDeviceByID(const DeviceID& id)
    {
        for (const auto& [domain, controller] : s_HostControllers)
        {
            auto device = controller->FindDeviceByID(id);

            if (device) return device;
        }

        return nullptr;
    }
}; // namespace PCI
