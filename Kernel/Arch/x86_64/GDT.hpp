/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

struct [[gnu::packed]] TaskStateSegment
{
    u32 Reserved1;
    u64 RSP0;
    u64 RSP1;
    u64 RSP2;
    u64 Reserved2;
    u64 IST1;
    u64 IST2;
    u64 IST3;
    u64 IST4;
    u64 IST5;
    u64 IST6;
    u64 IST7;
    u64 Reserved3;
    u16 Reserved4;
    u16 IoMapBase;
};

namespace GDT
{
    constexpr usize DPL_RING3              = 0x03ull;

    constexpr usize KERNEL_CODE_SELECTOR   = 0x08ull;
    constexpr usize KERNEL_DATA_SELECTOR   = 0x10ull;
    constexpr usize USERLAND_DATA_SELECTOR = 0x18ull;
    constexpr usize USERLAND_CODE_SELECTOR = 0x20ull;

    constexpr usize TSS_SELECTOR           = 0x28;

    void            Initialize();
    void            Load(u64 id);

    void            LoadTSS(TaskStateSegment* tss);
}; // namespace GDT
