/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/ACPI/ACPI.hpp>

constexpr const char* MCFG_SIGNATURE = "MCFG";
struct MCFG
{
    struct Entry
    {
        u64 Address;
        u16 Segment;
        u8  StartBus;
        u8  EndBus;
        u32 Reserved;
    } __attribute__((packed));

    SDTHeader Header;
    u64       Reserved;
    Entry     Entries[];
} __attribute__((packed));
