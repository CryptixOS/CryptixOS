/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/ACPI/ACPI.hpp>
#include <Prism/Containers/Vector.hpp>

namespace MADT
{
    struct [[gnu::packed]] Header
    {
        u8 ID;
        u8 Length;
    };
    struct [[gnu::packed]] LapicEntry
    {
        Header Header;
        u8     ProcessorID;
        u8     ApicID;
        u32    Flags;
    };
    struct [[gnu::packed]] IoApicEntry
    {
        Header Header;
        u8     ApicID;
        u8     Reserved;
        u32    Address;
        u32    GsiBase;
    };
    struct [[gnu::packed]] IsoEntry
    {
        Header Header;
        u8     BusSource;
        u8     IrqSource;
        u32    Gsi;
        u16    Flags;
    };
    struct [[gnu::packed]] LapicNmiEntry
    {
        Header Header;
        u8     Processor;
        u16    Flags;
        u8     Lint;
    };
    struct [[gnu::packed]] GenericEntry
    {
        union
        {
            LapicEntry    Lapic;
            IoApicEntry   IoApic;
            IsoEntry      Iso;
            LapicNmiEntry LapicNmi;
        } Entry;
    };

    void                   Initialize();
    bool                   LegacyPIC();

    Vector<LapicEntry>&    GetLapicEntries();
    Vector<IoApicEntry>&   GetIoApicEntries();
    Vector<IsoEntry>&      GetIsoEntries();
    Vector<LapicNmiEntry>& GetLapicNmiEntries();
} // namespace MADT
