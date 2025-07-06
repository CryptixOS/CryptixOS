/*
 * Created by v1tr10l7 on 16.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootInfo.hpp>
#include <Common.hpp>

#include <Firmware/DMI/SMBIOS.hpp>
#include <Firmware/DMI/Tables.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>

#include <magic_enum/magic_enum.hpp>

namespace DMI::SMBIOS
{
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

    static const char* DmiString(struct Header* header, u8 index)
    {
        char* current = reinterpret_cast<char*>(header);
        current += header->Length;

        if (!index) return "";
        index--;

        while (index > 0 && *current)
        {
            current += std::strlen(current);
            current++;
            index--;
        }

        return current;
    }
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

        usize   structureTableSize = entryPoint->StructureTableLength;
        Pointer structureTablePhys = entryPoint->StructureTableAddress;
        // TODO(v1tr10l7): We should allocate virtual address probably
        //  uintptr_t structureTableVirt = VMM::AllocateSpace(
        //      Math::AlignUp(structureTableSize, PMM::PAGE_SIZE), 0, false);
        Pointer structureTableVirt = structureTablePhys;
        VMM::GetKernelPageMap()->MapRange(
            structureTableVirt, structureTablePhys, structureTableSize * 2);

        u8* tableStart = structureTableVirt.As<u8>();
        u8* tableEnd   = structureTableVirt
                           .Offset<Pointer>(entryPoint->StructureTableLength)
                           .As<u8>();

        LogTrace("DMI: Enumerating SMBIOS entries...");
        while (tableStart < tableEnd)
        {
            Header* header = reinterpret_cast<Header*>(tableStart);

            LogTrace("DMI: Found SMBIOS table entry {:#x} - {}",
                     ToUnderlying(header->Type),
                     magic_enum::enum_name(header->Type));

            switch (header->Type)
            {
                case HeaderType::eBiosInformation:
                {
                    auto firmwareInfo
                        = reinterpret_cast<FirmwareInformation*>(header);

                    LogTrace("DMI: Table[{:#02x}] =>",
                             ToUnderlying(header->Type));
                    LogTrace("BIOS Vendor: {}",
                             DmiString(header, firmwareInfo->Vendor));
                    break;
                }
                case HeaderType::eEndOfTable: goto end;

                default: break;
            }

            usize structLength = CalculateStructLength(*header);
            tableStart += structLength;
        }

    end:
        LogInfo("SmBios: Initialized");
    }
}; // namespace DMI::SMBIOS
