/*
 * Created by v1tr10l7 on 15.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/MemoryZone.hpp>

enum class PagingMode
{
    eNone   = 0,
    eLevel4 = 4,
    eLevel5 = 5,
};
struct MemoryMap
{
    MemoryZone* Entries    = nullptr;
    usize       EntryCount = 0;
};
struct EfiMemoryMap
{
    Pointer Address           = nullptr;
    usize   Size              = 0;
    usize   DescriptorSize    = 0;
    u32     DescriptorVersion = 0;
};

struct BootMemoryInfo
{
    Pointer             KernelPhysicalBase = nullptr;
    Pointer             KernelVirtualBase  = nullptr;
    usize               HigherHalfOffset   = 0;
    enum PagingMode      PagingMode         = PagingMode::eNone;
    MemoryMap           MemoryMap          = {nullptr, 0};
    struct EfiMemoryMap EfiMemoryMap;
};
