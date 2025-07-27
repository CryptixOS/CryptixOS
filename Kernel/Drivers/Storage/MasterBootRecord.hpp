/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

struct [[gnu::packed]] MBREntry
{
    u8  Status;
    u8  StartCHS[3];
    u8  Type;
    u8  EndCHS[3];
    u32 FirstSector;
    u32 SectorCount;
};

constexpr usize SIGNATURE_OFFSET = 0x1fe;
constexpr u16   SIGNATURE        = 0xaa55;

struct [[gnu::packed]] MasterBootRecord
{
    MasterBootRecord() = default;

    u8       Code[446];
    MBREntry PartitionEntries[4];
    u16      Signature;
};

static_assert(sizeof(MasterBootRecord) == 512);
