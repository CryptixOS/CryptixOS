/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/EFI/Types.hpp>

namespace EFI
{
    enum MemoryAttribute : Uint64
    {
        eUC           = 0x0000000000000001,
        eWC           = 0x0000000000000002,
        eWT           = 0x0000000000000004,
        eWB           = 0x0000000000000008,
        eUCE          = 0x0000000000000010,
        eWP           = 0x0000000000001000,
        eRP           = 0x0000000000002000,
        eXP           = 0x0000000000004000,
        eNV           = 0x0000000000008000,
        eMoreReliable = 0x0000000000010000,
        eRO           = 0x0000000000020000,
        eSP           = 0x0000000000040000,
        eCpuCrypto    = 0x0000000000080000,
        eRuntime      = 0x8000000000000000,
        eIsaValid     = 0x4000000000000000,
        eIsaMask      = 0x0ffff00000000000,
    };
    enum MemoryType : Uint32
    {
        eReserved                = 0,
        eLoaderCode              = 1,
        eLoaderData              = 2,
        eBootServicesCode        = 3,
        eBootServicesData        = 4,
        eRuntimeServicesCode     = 5,
        eRuntimeServicesData     = 6,
        eConventionalMemory      = 7,
        eUnusableMemory          = 8,
        eAcpiReclaimableMemory   = 9,
        eAcpiNvs                 = 10,
        eMemoryMappedIo          = 11,
        eMemoryMappedIoPortSpace = 12,
        ePalCode                 = 13,
        ePersistentMemory        = 14,
        eUnacceptedMemory        = 15,
        eMaxMemoryType           = 16,
    };

    using PhysicalAddress = Uint64;
    using VirtualAddress  = Uint64;
    struct MemoryDescriptor
    {
        MemoryType      Type;
        Uint32          Padding;
        PhysicalAddress PhysicalStart;
        VirtualAddress  VirtualStart;
        Uint64          PageCount;
        MemoryAttribute Attribute;
    };

    struct MemoryMap
    {
        PhysicalAddress PhysMap;
        Uint64          Map;
        Uint64          MapEnd;
        Int32           MapEntryCount;
        Uint64          DescriptorVersion;
        Uint64          DescriptorSize;
        Uint64          Flags;
    };
}; // namespace EFI
