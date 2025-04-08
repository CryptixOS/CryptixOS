/*
 * Created by v1tr10l7 on 14.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/DeviceTree/Node.hpp>

#include <Prism/Memory/Endian.hpp>

constexpr usize FDT_MAGIC = 0xd00dfeed;

struct FDT_Header
{
    BigEndian<u32> Magic;
    BigEndian<u32> TotalSize;

    BigEndian<u32> StructureBlockOffset;
    BigEndian<u32> StringBlockOffset;
    BigEndian<u32> MemoryReservationBlockOffset;

    BigEndian<u32> Version;
    BigEndian<u32> LastCompatibleVersion;

    BigEndian<u32> BootCPUID;
    BigEndian<u32> StringBlockLength;
    BigEndian<u32> StructureBlockLength;
};

static_assert(sizeof(FDT_Header) == 40,
              "FDT header's size doesn't match the specification!");

namespace DeviceTree
{
    bool Initialize();
};
