/*
 * Created by vitriol1744 on 19.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/ACPI/ACPI.hpp>

namespace ACPI
{
    enum class AddressSpace : u8
    {
        eSystemMemory = 0,
        eSystemIO     = 1,
    };

    struct [[gnu::packed]] AddressStructure
    {
        AddressSpace AddressSpaceID;
        u8           RegisterBitWidth;
        u8           RegisterBitOffset;
        u8           Reserved;
        u64          Address;
    };

    namespace SDTs
    {
        struct [[gnu::packed]] HPET
        {
            SDTHeader        Header;
            u8               HardwareRevision;
            u8               ComparatorCount   : 5;
            u8               CounterSize       : 1;
            u8               Reserved          : 1;
            u8               LegacyReplacement : 1;
            u16              PCI_VendorID;
            AddressStructure EventTimerBlock;
            u8               SequenceNumber;
            u16              MinimumTick;
            u8               PageProtection : 4;
            u8               OemAttributes  : 4;
        };
    }; // namespace SDTs
};     // namespace ACPI
