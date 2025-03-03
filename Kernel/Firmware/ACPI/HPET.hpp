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
    namespace SDTs
    {
        struct [[gnu::packed]] HPET
        {
            SDTHeader               Header;
            u8                      HardwareRevision;
            u8                      ComparatorCount   : 5;
            u8                      CounterSize       : 1;
            u8                      Reserved          : 1;
            u8                      LegacyReplacement : 1;
            u16                     PCI_VendorID;
            GenericAddressStructure EventTimerBlock;
            u8                      SequenceNumber;
            u16                     MinimumTick;
            u8                      PageProtection : 4;
            u8                      OemAttributes  : 4;
        };
    }; // namespace SDTs
}; // namespace ACPI
