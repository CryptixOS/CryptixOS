/*
 * Created by v1tr10l7 on 16.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
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
#include <Prism/Memory/Pointer.hpp>
#include <Compiler.hpp>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
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
#include <Prism/String/StringUtils.hpp>

module Cryptix:Impl.Firmware.DMI.SMBIOS;

import :Debug.Assertions;
import :Library.Logger;
import :Library.Stacktrace;
import :Firmware.DMI.SMBIOS;
import :Firmware.DMI.Tables;
import :Memory.PMM;
import :Memory.ScopedMapping;
import :Memory.VMM;

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
            current += StringView(current).Size();
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

    void Initialize(Pointer entry32, Pointer entry64)
    {
        // TODO(v1tr10l7): Verify which entry should we use
        if (!entry32) return;

        LogTrace("DMI: 32 bit SMBIOS entry point => {:#x}", entry32);
        LogTrace("DMI: 64 bit SMBIOS entry point => {:#x}", entry64);
        ScopedMapping<EntryPoint32> entryPoint(
            entry32, PageAttributes::eRW | PageAttributes::eWriteBack);

        StringView anchorString(
            reinterpret_cast<const char*>(&entryPoint->AnchorString), 4);
        if (anchorString != "_SM_"_sv) return;

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

            LogTrace("DMI: Table[{:#02x}] =>", ToUnderlying(header->Type));
            switch (header->Type)
            {
                case HeaderType::eBiosInformation:
                {
                    break;
                    auto firmwareInfo
                        = reinterpret_cast<FirmwareInformation*>(header);

                    LogTrace("\tVendor: {}",
                             DmiString(header, firmwareInfo->Vendor));
                    LogTrace("\tVersion: {}",
                             DmiString(header, firmwareInfo->Version));
                    LogTrace("\tBIOS Start Segment: {}",
                             firmwareInfo->BiosStartSegment);
                    LogTrace("\tRelease Date: {}",
                             DmiString(header, firmwareInfo->ReleaseDate));
                    LogTrace("\tROM Size: {}", firmwareInfo->RomSize);

                    u8* raw
                        = reinterpret_cast<u8*>(&firmwareInfo->Characteristics);
                    LogTrace(
                        "Raw: {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} "
                        "{:02x}",
                        raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6],
                        raw[7]);
                    FirmwareCharacteristics& props
                        = firmwareInfo->Characteristics;
                    LogTrace("\tISA Supported: {}", (bool)props.ISA);
                    LogTrace("\tMCA Supported: {}", (bool)props.MCA);
                    LogTrace("\tEISA Supported: {}", (bool)props.EISA);
                    LogTrace("\tPCI Supported: {}", (bool)props.PCI);
                    LogTrace("\tPCMCIA Supported: {}", (bool)props.PCMCIA);
                    LogTrace("\tPNP Supported: {}", (bool)props.PNP);
                    LogTrace("\tAPM Supported: {}", (bool)props.APM);
                    LogTrace("\tUpgradable: {}", (bool)props.Upgradeable);
                    LogTrace("\tShadowing Supported: {}",
                             (bool)props.ShadowingAllowed);
                    LogTrace("\tVL-VESA Supported: {}", (bool)props.VLVESA);
                    LogTrace("\tESCD Supported: {}", (bool)props.ESCD);
                    LogTrace("\tBoot From CD Supported: {}",
                             (bool)props.BootFromCd);
                    LogTrace("\tSelectable Boot Supported: {}",
                             (bool)props.SelectableBoot);
                    LogTrace("\tROM Socketed: {}", (bool)props.ROMSocketed);
                    LogTrace("\tBoot From PC Card Supported: {}",
                             (bool)props.BootFromPCCard);
                    LogTrace("\tEDD Supported: {}", (bool)props.EDD);
                    LogTrace("\tNEC9800 Supported: {}", (bool)props.NEC9800);
                    LogTrace("\tToshiba1_2Mib Supported: {}",
                             (bool)props.Toshiba1_2MiB);
                    LogTrace("\tFloppy 360KB Supported: {}",
                             (bool)props.Floppy360KB);
                    LogTrace("\tFloppy 1.2MB Supported: {}",
                             (bool)props.Floppy1_2MB);
                    LogTrace("\tFloppy 720KB Supported: {}",
                             (bool)props.Floppy720KB);
                    LogTrace("\tPrint Service Supported: {}",
                             (bool)props.PrintService);
                    LogTrace("\tI8042 Keyboard Supported: {}",
                             (bool)props.I8042Keyboard);
                    LogTrace("\tSerial Services Supported: {}",
                             (bool)props.SerialServices);
                    LogTrace("\tPrinter Services Supported: {}",
                             (bool)props.PrinterServices);
                    LogTrace("\tCGA Supported: {}", (bool)props.CGA);
                    LogTrace("\tNECPC98 Supported: {}", (bool)props.NECPC98);
                    LogTrace("\tMajor Release: {}", firmwareInfo->MajorRelease);
                    LogTrace("\tMinor Release: {}", firmwareInfo->MinorRelease);
                    LogTrace("\tEmbedded Controller Major Release: {}",
                             firmwareInfo->EmbeddedControllerMajor);
                    LogTrace("\tEmbedded Controller Minor Release: {}",
                             firmwareInfo->EmbeddedControllerMinor);

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
