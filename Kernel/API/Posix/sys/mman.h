/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

constexpr usize     PROT_NONE      = 0x00;
constexpr usize     PROT_READ      = 0x01;
constexpr usize     PROT_WRITE     = 0x02;
constexpr usize     PROT_EXEC      = 0x04;

constexpr usize     MAP_FILE       = 0x00;
constexpr usize     MAP_SHARED     = 0x01;
constexpr usize     MAP_PRIVATE    = 0x02;
constexpr usize     MAP_TYPE       = 0x0f;
constexpr usize     MAP_FIXED      = 0x10;
constexpr usize     MAP_ANONYMOUS  = 0x20;

constexpr usize     MAP_HUGE_SHIFT = 26;
constexpr usize     MAP_HUGE_MASK  = 0x3f;

constexpr usize     MAP_HUGE_16KB  = (14 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_64KB  = (16 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_512KB = (19 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_1MB   = (20 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_2MB   = (21 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_8MB   = (23 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_16MB  = (24 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_32MB  = (25 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_256MB = (28 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_512MB = (29 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_1GB   = (30 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_2GB   = (31 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_16GB  = (34u << MAP_HUGE_SHIFT);

constexpr uintptr_t MAP_FAILED     = -1;
