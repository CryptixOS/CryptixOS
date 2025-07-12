/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/ACPI/Structures.hpp>

namespace ACPI::SRAT
{
    /*******************************************************************************
     *
     * SRAT - System Resource Affinity Table
     *        Version 3
     *
     ******************************************************************************/
    struct SRAT
    {
        SDTHeader Header;
        u32       Revision;
        u64       Reserved;
    };

    enum class Type : u8
    {
        eCPUAffinity         = 0,
        eMemoryAffinity      = 1,
        eX2ApicAffinity      = 2,
        eGiccAffinity        = 3,
        eGicItsAffinity      = 4,
        eGenericAffinity     = 5,
        eGenericPortAffinity = 6,
        eRintcAffinity       = 7,
        eReserved            = 8,
    };

    /* 0: Processor Local APIC/SAPIC Affinity */
    constexpr usize CPU_AFFINITY_ENABLED = Bit(0);
    struct CPU_Affinity
    {
        enum Type Type;
        u8        Length;
        u8        ProximityDomainLow;
        u8        ApicID;
        u32       Flags;
        u8        LocalSapicEID;
        u8        ProximityDomainHigh[3];
        u32       ClockDomain;
    };
    /* 1: Memory Affinity */
    constexpr usize MEMORY_AFFINITY_ENABLED       = Bit(0);
    constexpr usize MEMORY_AFFINITY_HOT_PLUGGABLE = Bit(1);
    constexpr usize MEMORY_AFFINITY_NON_VOLATILE  = Bit(2);
    struct MemoryAffinity
    {
        enum Type Type;
        u8        Length;
        u32       ProximityDomain;
        u16       Reserved;
        u64       BaseAddress;
        u64       LengthLow;
        u32       Reserved1;
        u32       Flags;
        u64       Reserved2;
    };
    /* 2: Processor Local X2_APIC Affinity (ACPI 4.0) */
    struct X2ApicAffinity
    {
        enum Type Type;
        u8        Length;
        u16       Reserved;
        u32       ProximityDomain;
        u32       ApicID;
        u32       Flags;
        u32       ClockDomain;
        u32       Reserved2;
    };

    /* 3: GICC Affinity (ACPI 5.1) */
    constexpr usize GICC_AFFINITY_ENABLED = Bit(0);
    struct GiccAffinity
    {
        enum Type Type;
        u8        Length;
        u32       ProximityDomain;
        u32       ProcessorUID;
        u32       Flags;
        u32       ClockDomain;
    };

    /* 4: GIC ITS Affinity (ACPI 6.2) */
    struct GicItsAffinity
    {
        enum Type Type;
        u8        Length;
        u32       ProximityDomain;
        u16       Reserved;
        u32       its_id;
    };

    constexpr usize DEVICE_HANDLE_SIZE        = 16;
    constexpr usize GENERIC_AFFINITY_HID_MASK = 0xffff'ffff'ffff'ffff;
    constexpr usize GENERIC_AFFINITY_UID_MASK = 0xffff'ffff'ffff'ffff << 4;
    struct GenericAffinity
    {
        enum Type Type;
        u8        Length;
        u8        Reserved;
        u8        DeviceHandleType;
        u32       ProximityDomain;
        u8        DeviceHandle[DEVICE_HANDLE_SIZE];
        u32       Flags;
        u32       Reserved1;
    };

    /* 7: RINTC Affinity Structure(ACPI 6.6) */
    struct RintcAffinity
    {
        enum Type Type;
        u8        Length;
        u16       Reserved;
        u32       ProximityDomain;
        u32       ProcessorUID;
        u32       Flags;
        u32       ClockDomain;
    };
}; // namespace ACPI::SRAT
