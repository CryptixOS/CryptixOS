/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <ACPI/ACPI.hpp>

#include <vector>

namespace MADT
{
    struct Header
    {
        u8 ID;
        u8 Length;
    } __attribute__((packed));

    struct LapicEntry
    {
        Header Header;
        u8     ProcessorID;
        u8     ApicID;
        u32    Flags;
    } __attribute__((packed));

    struct IoApicEntry
    {
        Header Header;
        u8     ApicID;
        u8     Reserved;
        u32    Address;
        u32    GsiBase;
    } __attribute__((packed));

    struct IsoEntry
    {
        Header Header;
        u8     BusSource;
        u8     IrqSource;
        u32    Gsi;
        u16    Flags;
    } __attribute__((packed));

    struct LapicNmiEntry
    {
        Header Header;
        u8     Processor;
        u16    Flags;
        u8     Lint;
    } __attribute__((packed));

    void                         Initialize();
    bool                         LegacyPIC();

    std::vector<LapicEntry*>&    GetLapicEntries();
    std::vector<IoApicEntry*>&   GetIoApicEntries();
    std::vector<IsoEntry*>&      GetIsoEntries();
    std::vector<LapicNmiEntry*>& GetLapicNmiEntries();
} // namespace MADT
