/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
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
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Compiler.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Containers/Vector.hpp>

export module Cryptix:Firmware.ACPI.MADT;

export import :Firmware.ACPI.ACPI;

export namespace MADT
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
