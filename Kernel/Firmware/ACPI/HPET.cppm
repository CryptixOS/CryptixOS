/*
 * Created by vitriol1744 on 19.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <Vitriol1744@gmail.com>
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

export module Cryptix:Firmware.ACPI.HPET;

export import :Firmware.ACPI.ACPI;

namespace ACPI
{
    export namespace SDTs
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
