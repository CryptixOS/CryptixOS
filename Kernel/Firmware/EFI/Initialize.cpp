/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootInfo.hpp>
#include <Debug/Debug.hpp>

#include <Firmware/EFI/Memory.hpp>
#include <Firmware/EFI/SystemTable.hpp>
#include <Library/Logger.hpp>

#include <Memory/VMM.hpp>
#include <Prism/String/StringUtils.hpp>

namespace EFI
{
    SystemTable* g_SystemTable = nullptr;

    bool         Initialize(Pointer systemTable, const EfiMemoryMap& memoryMap)
    {
        LogTrace("EFI: Initializing...");
        if (!systemTable)
        {
            LogTrace("EFI: System table not available, aborting");
            return false;
        }

        g_SystemTable = systemTable.As<SystemTable>();
        if (!memoryMap.Address)
        {
            LogError(
                "EFI: Couldn't retrieve "
                "the memory map");
            return false;
        }

        auto    memoryMapSize  = memoryMap.Size;
        auto    descriptorSize = memoryMap.DescriptorSize;
        usize   entryCount     = memoryMapSize / descriptorSize;

        Pointer current        = memoryMap.Address;
        Pointer end            = current.Offset(memoryMapSize);
        if (!current || entryCount == 0)
        {
            return false;
            LogError("EFI: Couldn't acquire efi memory map");
        }

        LogTrace(
            "EFI: Enumerating efi memory map, and mapping runtime services "
            "regions...");
        LogTrace("EFI: Memory map entry count: {}", entryCount);

        bool runtimeCodeMapped = false;
        bool runtimeDataMapped = false;
        for (usize i = 0; current < end; current += descriptorSize, i++)
        {
            auto       descriptor = current.As<MemoryDescriptor>();
            MemoryType type       = descriptor->Type;
            usize      phys       = descriptor->PhysicalStart;
            usize      virt       = descriptor->VirtualStart;
            usize      pageCount  = descriptor->PageCount;

            StringView typeString = ToString(type);
            if (typeString.Empty()) typeString = "Unknown"_sv;
            if (typeString.StartsWith("e")) typeString.RemovePrefix(1);

#if CTOS_DUMP_EFI_MEMORY_MAP
            auto attributes = ToUnderlying(descriptor->Attribute);

            LogTrace(
                "EFI: MemoryMap[{}] => {{ .Type: {}, .PhysicalStart: {:#x}, "
                ".VirtualStart: {:#x}, .PageCount: {}, .Attributes: {:#b} }}",
                i, typeString, phys, virt, pageCount, attributes);
#endif

            if (type != MemoryType::eRuntimeServicesCode
                && type != MemoryType::eRuntimeServicesData)
                continue;

            auto pageAttributes
                = PageAttributes::eRead | PageAttributes::eWriteBack;
            if (type == MemoryType::eRuntimeServicesCode)
                pageAttributes |= PageAttributes::eExecutable;
            else pageAttributes |= PageAttributes::eWrite;

            // TODO(v1tr10l7): Allocate virtual region for the efi runtime
            // services, and relocate them to it
            if (!VMM::MapKernelRegion(virt, phys, pageCount, pageAttributes))
            {
                LogError("EFI: Failed to map '{}' region", typeString);
                return false;
            }

            if (type == MemoryType::eRuntimeServicesCode)
                runtimeCodeMapped = true;
            else runtimeDataMapped = true;
        }

        return (runtimeCodeMapped && runtimeDataMapped);
    }
}; // namespace EFI
