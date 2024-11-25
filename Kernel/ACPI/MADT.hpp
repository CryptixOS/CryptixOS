/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "ACPI/ACPI.hpp"

#include <cstdint>
#include <vector>

namespace MADT
{
#pragma pack(push, 1)
    struct Header
    {
        u8 id;
        u8 length;
    };

    struct LAPICEntry
    {
        Header header;
        u8     processorID;
        u8     apicID;
        u32    flags;
    };

    struct IOAPICEntry
    {
        Header header;
        u8     apicID;
        u8     reserved;
        u32    address;
        u32    gsib;
    };

    struct ISOEntry
    {
        Header header;
        u8     busSource;
        u8     irqSource;
        u32    gsi;
        u16    flags;
    };

    struct LAPIC_NMIEntry
    {
        Header header;
        u8     processor;
        u16    flags;
        u8     lint;
    };
#pragma pack(pop)

    void                          Initialize();
    bool                          LegacyPIC();

    std::vector<LAPICEntry*>&     GetLAPICEntries();
    std::vector<IOAPICEntry*>&    GetIOAPICEntries();
    std::vector<ISOEntry*>&       GetISOEntries();
    std::vector<LAPIC_NMIEntry*>& GetLAPIC_NMIEntries();
} // namespace MADT
