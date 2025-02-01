/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

struct MBREntry
{
    u8  Status;
    u8  StartCHS[3];
    u8  Type;
    u8  EndCHS[3];
    u32 FirstSector;
    u32 SectorCount;
} __attribute__((packed));

constexpr usize SIGNATURE_OFFSET = 0x1fe;
constexpr u16   SIGNATURE        = 0xaa55;

struct MasterBootRecord
{
    u8       Code[446];
    MBREntry PartitionEntries[4];
    u16      Signature;
} __attribute__((packed));

static_assert(sizeof(MasterBootRecord) == 512);
