/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootInfo.hpp>

#include <Firmware/EFI/Memory.hpp>
#include <Library/Logger.hpp>

#include <Memory/VMM.hpp>

#include <magic_enum/magic_enum.hpp>

namespace EFI
{
    bool Initialize()
    {
        auto firmwareType = BootInfo::FirmwareType();
        if (firmwareType != FirmwareType::eUefi32
            && firmwareType != FirmwareType::eUefi64)
            return false;

        LogTrace("EFI: Initializing...");
        auto efiMemoryMapResponse = BootInfo::EfiMemoryMap();
        if (!efiMemoryMapResponse)
        {
            LogError(
                "EFI: Couldn't retrieve "
                "limine_efi_memmap_response");
            return false;
        }

        Pointer memoryMap      = efiMemoryMapResponse->memmap;
        auto    memoryMapSize  = efiMemoryMapResponse->memmap_size;
        auto    descriptorSize = efiMemoryMapResponse->desc_size;
        usize   entryCount     = memoryMapSize / descriptorSize;

        Pointer current        = efiMemoryMapResponse->memmap;
        Pointer end            = memoryMap.Offset(memoryMapSize);
        if (!memoryMap || entryCount == 0)
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
            auto       attributes = ToUnderlying(descriptor->Attribute);

            StringView typeString
                = magic_enum::enum_name(type).data() ?: "Unknown";
            if (typeString.StartsWith("e")) typeString = typeString.Raw() + 1;

            LogTrace(
                "EFI: MemoryMap[{}] => {{ .Type: {}, .PhysicalStart: {:#x}, "
                ".VirtualStart: {:#x}, .PageCount: {}, .Attributes: {:#b} }}",
                i, typeString, phys, virt, pageCount, attributes);

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
