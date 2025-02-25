/*
 * Created by vitriol1744 on 20.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <magic_enum/magic_enum.hpp>

namespace ACPI
{
    struct [[gnu::packed]] SDTHeader
    {
        char Signature[4];
        u32  Length;
        u8   Revision;
        u8   Checksum;
        char OemID[6];
        u64  OemTableID;
        u32  OemRevision;
        u32  CreatorID;
        u32  CreatorRevision;
    };

    enum class AddressSpace : u8
    {
        eSystemMemory                 = 0,
        eSystemIO                     = 1,
        ePciConfigurationSpace        = 2,
        eEmbeddedControler            = 3,
        eSMBus                        = 4,
        eSystemCMOS                   = 5,
        ePciDeviceBar                 = 6,
        eIPMI                         = 7,
        eGeneralPurposeIO             = 8,
        eGenericSerialBus             = 9,
        ePlatformCommunicationChannel = 0xa,
    };
    struct [[gnu::packed]] GenericAddressStructure
    {
        AddressSpace AddressSpaceID;
        u8           RegisterBitWidth;
        u8           RegisterBitOffset;
        u8           Reserved;
        u64          Address;

        void         Write(u32 value)
        {
            switch (AddressSpaceID)
            {
                case AddressSpace::eSystemMemory:
                case AddressSpace::eSystemIO:
                case AddressSpace::ePciConfigurationSpace:
                case AddressSpace::eEmbeddedControler:
                case AddressSpace::eSMBus:
                case AddressSpace::eSystemCMOS:
                case AddressSpace::ePciDeviceBar:
                case AddressSpace::eIPMI:
                case AddressSpace::eGeneralPurposeIO:
                case AddressSpace::eGenericSerialBus:
                case AddressSpace::ePlatformCommunicationChannel:

                default:
                    Panic(
                        "GenericAddressStructure: Writing to '{}' address "
                        "space is not implemented!",
                        magic_enum::enum_name(AddressSpaceID).data() + 1);
                    break;
            }
        }
    };

    struct [[gnu::packed]] FADT
    {
        SDTHeader               Header;
        u32                     FirmwareControl;
        u32                     DSDT;
        u8                      Reserved;
        u8                      PrefferedPmProfile;
        u16                     SciInterrupt;
        u32                     SmiCmd;
        u8                      AcpiEnable;
        u8                      AcpiDisable;
        u8                      S4BiosReq;
        u8                      PStateControl;
        u32                     PM1aEventBlock;
        u32                     PM1bEventBlock;
        u32                     PM1aControlBlock;
        u32                     PM1bControlBlock;
        u32                     PM2ControlBlock;
        u32                     PMTimerBlock;
        u32                     GPE0Block;
        u32                     GPE1Block;
        u8                      PM1EventLength;
        u8                      PM1ControlLength;
        u8                      PM2ControlLength;
        u8                      PMTimerLength;
        u8                      GPE0Length;
        u8                      GPE1Length;
        u8                      GPE1Base;
        u8                      CStateControl;
        u16                     WorstC2Latency;
        u16                     WorstC3Latency;
        u16                     FlushSize;
        u16                     FlushStride;
        u8                      DutyOffset;
        u8                      DutyWidth;
        u8                      DayAlarm;
        u8                      MonthAlarm;
        u8                      Century;

        // reserved in ACPI 1.0; used since ACPI 2.0+
        u16                     BootArchitectureFlags;

        u8                      Reserved2;
        u32                     Flags;

        // 12 byte structure; see below for details
        GenericAddressStructure ResetReg;

        u8                      ResetValue;
        u8                      Reserved3[3];

        // 64bit pointers - Available on ACPI 2.0+
        u64                     X_FirmwareControl;
        u64                     X_Dsdt;

        GenericAddressStructure X_PM1aEventBlock;
        GenericAddressStructure X_PM1bEventBlock;
        GenericAddressStructure X_PM1aControlBlock;
        GenericAddressStructure X_PM1bControlBlock;
        GenericAddressStructure X_PM2ControlBlock;
        GenericAddressStructure X_PMTimerBlock;
        GenericAddressStructure X_GPE0Block;
        GenericAddressStructure X_GPE1Block;
    };
}; // namespace ACPI
