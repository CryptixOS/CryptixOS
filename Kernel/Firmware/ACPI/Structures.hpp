/*
 * Created by vitriol1744 on 20.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#ifdef CTOS_TARGET_X86_64
    #include <Arch/x86_64/IO.hpp>
#endif

#include <Common.hpp>
#include <Memory/MMIO.hpp>

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
        eEmbeddedController           = 3,
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
        u8           AccessSize;
        u64          Address;

        void         Write(u32 value)
        {
            switch (AddressSpaceID)
            {
                case AddressSpace::eSystemMemory:
                    switch (AccessSize)
                    {
                        case 1: return MMIO::Write<u8>(Address, value);
                        case 2: return MMIO::Write<u16>(Address, value);
                        case 3: return MMIO::Write<u32>(Address, value);
                        case 4: return MMIO::Write<u64>(Address, value);

                        default: AssertNotReached();
                    }
                case AddressSpace::eSystemIO:
#ifdef CTOS_TARGET_X86_64
                    switch (RegisterBitWidth)
                    {
                        case 8: return IO::Out<u8>(Address, value);
                        case 16: return IO::Out<u16>(Address, value);
                        case 32: return IO::Out<u32>(Address, value);

                        default: AssertNotReached();
                    }
#else
                    AssertNotReached();
#endif
                case AddressSpace::ePciConfigurationSpace:
                case AddressSpace::eEmbeddedController:
                case AddressSpace::eSMBus:
                case AddressSpace::eSystemCMOS:
                case AddressSpace::ePciDeviceBar:
                case AddressSpace::eIPMI:
                case AddressSpace::eGeneralPurposeIO:
                case AddressSpace::eGenericSerialBus:
                case AddressSpace::ePlatformCommunicationChannel:

                default:
                    LogWarn(
                        "GenericAddressStructure: Writing to '{}' address "
                        "space is not implemented!",
                        magic_enum::enum_name(AddressSpaceID).data() + 1);
                    break;
            }
        }
    };

    struct [[gnu::packed]] X86BootArchitectureFlags
    {
        bool LegacyDevices     : 1  = 0;
        bool I8042Available    : 1  = 0;
        bool VgaNotPresent     : 1  = 0;
        bool MsiNotSupported   : 1  = 0;
        bool PCIeASPM_Controls : 1  = 0;
        bool CmosRtcNotPresent : 1  = 0;
        u16  Reserved          : 10 = 0;
    };
    enum class FadtFlags : u32
    {
        eNone                  = 0,
        // WBINVD instruction is operational
        eWbinvd                = Bit(0),
        // WBINVD flushes caches properly
        eWbinvdFlush           = Bit(1),
        // C1 power state available
        eSupportsC1            = Bit(2),
        // C2 power state available
        eSupportsC2            = Bit(3),
        // Power button is a control method device
        ePowerButton           = Bit(4),
        // Sleep button is a control method device
        eSleepButton           = Bit(5),
        // RTC can wake system
        eFixedRtcWake          = Bit(6),
        // RTC can wake from S4
        eRtcCanWakeS4          = Bit(7),

        // 32-bit PM timer
        eTmrIs32Bit            = Bit(8),
        // System supports docking
        eDockingSupported      = Bit(9),
        // Reset register is valid
        eResetRegSupported     = Bit(10),
        // System is sealed
        eSealedCase            = Bit(11),
        // Headless system
        eHeadlessSystem        = Bit(12),
        // Use software-based sleep control
        eSoftwareSlp           = Bit(13),
        // PCIe wake supported
        ePciExpressWake        = Bit(14),
        // Use platform-provided clock source
        eUsePlatformClock      = Bit(15),

        // RTC status valid for S4
        eRtcStsValidOnS4       = Bit(16),
        // Supports remote wake
        eRemoteWakeSupported   = Bit(17),
        // Force APIC cluster model
        eForceApicClusterModel = Bit(18),
        // Force physical destination mode
        eForceApicPhysicalDest = Bit(19),

        // Hardware-reduced ACPI model
        eHardwareReducedAcpi   = Bit(20),
        // Low Power S0 Idle (Modern Standby)
        eLowPowerS0Idle        = Bit(21),
    };

    inline constexpr FadtFlags operator|(FadtFlags a, FadtFlags b)
    {
        return static_cast<FadtFlags>(static_cast<u32>(a)
                                      | static_cast<u32>(b));
    }

    inline constexpr FadtFlags operator&(FadtFlags a, FadtFlags b)
    {
        return static_cast<FadtFlags>(static_cast<u32>(a)
                                      & static_cast<u32>(b));
    }
    inline constexpr FadtFlags operator~(FadtFlags a)
    {
        return static_cast<FadtFlags>(~static_cast<u32>(a));
    }
    inline constexpr bool operator==(FadtFlags lhs, usize rhs)
    {
        return ToUnderlying(lhs) == rhs;
    }
    inline constexpr bool operator!=(FadtFlags lhs, usize rhs)
    {
        return ToUnderlying(lhs) != rhs;
    }

    struct [[gnu::packed]] FADT
    {
        SDTHeader                Header;
        u32                      FirmwareControl;
        u32                      DSDT;
        u8                       Reserved;
        u8                       PreferredPmProfile;
        u16                      SciInterrupt;
        u32                      SmiCmd;
        u8                       AcpiEnable;
        u8                       AcpiDisable;
        u8                       S4BiosReq;
        u8                       PStateControl;
        u32                      PM1aEventBlock;
        u32                      PM1bEventBlock;
        u32                      PM1aControlBlock;
        u32                      PM1bControlBlock;
        u32                      PM2ControlBlock;
        u32                      PMTimerBlock;
        u32                      GPE0Block;
        u32                      GPE1Block;
        u8                       PM1EventLength;
        u8                       PM1ControlLength;
        u8                       PM2ControlLength;
        u8                       PMTimerLength;
        u8                       GPE0Length;
        u8                       GPE1Length;
        u8                       GPE1Base;
        u8                       CStateControl;
        u16                      WorstC2Latency;
        u16                      WorstC3Latency;
        u16                      FlushSize;
        u16                      FlushStride;
        u8                       DutyOffset;
        u8                       DutyWidth;
        u8                       DayAlarm;
        u8                       MonthAlarm;
        u8                       Century;

        // reserved in ACPI 1.0; used since ACPI 2.0+
        X86BootArchitectureFlags X86BootArchitectureFlags;

        u8                       Reserved2;
        FadtFlags                Flags;

        // 12 byte structure; see below for details
        GenericAddressStructure  ResetReg;

        u8                       ResetValue;
        u8                       Reserved3[3];

        // 64bit pointers - Available on ACPI 2.0+
        u64                      X_FirmwareControl;
        u64                      X_Dsdt;

        GenericAddressStructure  X_PM1aEventBlock;
        GenericAddressStructure  X_PM1bEventBlock;
        GenericAddressStructure  X_PM1aControlBlock;
        GenericAddressStructure  X_PM1bControlBlock;
        GenericAddressStructure  X_PM2ControlBlock;
        GenericAddressStructure  X_PMTimerBlock;
        GenericAddressStructure  X_GPE0Block;
        GenericAddressStructure  X_GPE1Block;
    };
}; // namespace ACPI
