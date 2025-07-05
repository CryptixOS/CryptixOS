/*
 * Created by v1tr10l7 on 16.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Firmware/SMBIOS/SMBIOS.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Boot/BootInfo.hpp>
#include <Prism/Utility/Math.hpp>

#include <magic_enum/magic_enum.hpp>

namespace SMBIOS
{
    namespace
    {
        enum class HeaderType : u8
        {
            eBiosInformation         = 0,
            eSystemInformation       = 1,
            eMainboardInformation    = 2,
            eChassisInformation      = 3,
            eProcessorInformation    = 4,
            eCacheInformation        = 7,
            eSystemSlotsInformation  = 9,
            ePhysicalMemoryArray     = 16,
            eMemoryDeviceInformation = 17,
            eMemoryArrayMappedAddr   = 19,
            eSystemBootInformation   = 32,
        };

        struct [[gnu::packed]] SystemInformation
        {
            u8  SystemInformationIndicator;
            u8  Length;
            u16 Handle;
            u8  ManufacturerStringID;
            u8  ProductNameStringID;
            u8  VersionStringID;
            u8  SerialNumberStringID;
            u16 UUID;
            u8  WakeUpType;
            u16 SkuNumberStringID;
        };

        struct [[gnu::packed]] EntryPoint32
        {
            u32 AnchorString;
            u8  Checksum;
            u8  Length;
            u8  Major;
            u8  Minor;
            u16 StructureMaxSize;
            u8  Revision;
            u8  FormattedArea[5];
            u8  IntermediateAnchorString[5];
            u8  IntermediateChecksum;
            u16 StructureTableLength;
            u32 StructureTableAddress;
            u16 StructureCount;
            u8  BcdRevision;
        };
        struct [[gnu::packed]] EntryPoint64
        {
            u8  AnchorString[5];
            u8  Checksum;
            u8  Length;
            u8  Major;
            u8  Minor;
            u8  DocRevision;
            u8  Revision;
            u8  Reserved;
            u32 StructureTableMaxSize;
            u64 StructureTableAddress;
        };

        struct [[gnu::packed]] Header
        {
            HeaderType Type;
            u8         Length;
            u16        Handle;
        };

        void ParseEntry(Header& header) {}
    }; // namespace

    static usize CalculateStructLength(Header& header)
    {
        const char* strTab = reinterpret_cast<char*>(
            reinterpret_cast<uintptr_t>(&header) + header.Length);

        usize i = 1;
        for (i = 1; strTab[i - 1] != '\0' || strTab[i] != '\0'; i++)
            Arch::Pause();

        return header.Length + i + 1;
    }

    void Initialize()
    {
        auto smbiosEntries = BootInfo::GetSmBiosEntries();
        // TODO(v1tr10l7): Verify which entry should we use
        if (!smbiosEntries.first) return;

        Pointer entryPointVirt   = smbiosEntries.first.ToHigherHalf<Pointer>();
        EntryPoint32* entryPoint = entryPointVirt.As<EntryPoint32>();
        if (std::strncmp(
                reinterpret_cast<const char*>(&entryPoint->AnchorString),
                "_SM_", 4)
            != 0)
            return;

        char* anchorString = reinterpret_cast<char*>(&entryPoint->AnchorString);
        LogTrace("AnchorString: {}{}{}{}", anchorString[0], anchorString[1],
                 anchorString[2], anchorString[3]);
        LogTrace("Checksum: {}", entryPoint->Checksum);
        LogTrace("Length: {}", entryPoint->Length);
        LogTrace("Major: {}", entryPoint->Major);
        LogTrace("Minor: {}", entryPoint->Minor);
        LogTrace("StructureMaxSize: {}", entryPoint->StructureMaxSize);
        LogTrace("Revision: {}", entryPoint->Revision);
        LogTrace("StructureTableLength: {}", entryPoint->StructureTableLength);
        LogTrace("StructureTableCount: {}", entryPoint->StructureCount);
        LogTrace("StructureTableAddress: {:#x}",
                 entryPoint->StructureTableAddress);

        usize     structureTableSize = entryPoint->StructureTableLength;
        Pointer   structureTablePhys = entryPoint->StructureTableAddress;
        // TODO(v1tr10l7): We should allocate virtual address probably
        //  uintptr_t structureTableVirt = VMM::AllocateSpace(
        //      Math::AlignUp(structureTableSize, PMM::PAGE_SIZE), 0, false);
        uintptr_t structureTableVirt = structureTablePhys;
        VMM::GetKernelPageMap()->MapRange(
            structureTableVirt, structureTablePhys, structureTableSize * 2);

        u8* tableStart = Pointer(structureTableVirt).As<u8>();
        u8* tableEnd   = Pointer(structureTableVirt)
                           .Offset<Pointer>(entryPoint->StructureTableLength)
                           .As<u8>();

        LogTrace("TableStartVirt: {:#x}",
                 reinterpret_cast<uintptr_t>(tableStart));
        LogTrace("SmBios: Enumerating SmBios entries...");
        while (tableStart < tableEnd)
        {
            Header* header = reinterpret_cast<Header*>(tableStart);

            LogTrace("SMBIOS: Found entry {:#x} - {}", usize(header->Type),
                     magic_enum::enum_name(header->Type));
            ParseEntry(*header);

            if (header->Type == static_cast<HeaderType>(127)) break;

            usize structLength = CalculateStructLength(*header);

            tableStart += structLength;
        }

        LogInfo("SmBios: Initialized");
    }
}; // namespace SMBIOS
