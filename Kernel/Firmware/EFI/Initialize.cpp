/*
 * Created by v1tr10l7 on 19.04.2025.
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
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Debug/Debug.hpp>
#include <Firmware/EFI/Types.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/String/StringUtils.hpp>

module Cryptix:Impl.Firmware.EFI.Initialize;

import :Boot.BootMemoryInfo;
import :Firmware.EFI.Memory;
import :Firmware.EFI.SystemTable;
import :Library.Logger;
import :Memory.VMM;

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
            LogError("EFI: Couldn't acquire efi memory map");
            return false;
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
            auto pageMap = VMM::GetKernelPageMap();

            IgnoreUnused(pageCount);
            IgnoreUnused(phys);
            if (!pageMap->Protect(virt, pageAttributes))
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
