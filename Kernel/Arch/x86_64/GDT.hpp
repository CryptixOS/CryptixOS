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
    u32 reserved1;
    u64 rsp[3];
    u64 reserved2;
    u64 ist[7];
    u64 reserved3;
    u16 reserved4;
    u16 ioMapBase;
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
