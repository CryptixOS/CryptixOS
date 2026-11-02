/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
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

export module Cryptix:Firmware.ACPI.MCFG;

export import :Firmware.ACPI.ACPI;

export constexpr const char* MCFG_SIGNATURE = "MCFG";
export struct MCFG
{
    struct Entry
    {
        u64 Address;
        u16 Segment;
        u8  StartBus;
        u8  EndBus;
        u32 Reserved;
    } __attribute__((packed));

    ACPI::SDTHeader Header;
    u64             Reserved;
    Entry           Entries[];
} __attribute__((packed));
