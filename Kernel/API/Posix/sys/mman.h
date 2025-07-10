/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

constexpr usize     PROT_NONE       = 0x00;
constexpr usize     PROT_READ       = 0x01;
constexpr usize     PROT_WRITE      = 0x02;
constexpr usize     PROT_EXEC       = 0x04;

constexpr uintptr_t MAP_FAILED      = -1;

// map file
constexpr usize     MAP_FILE        = 0x00;
// share changes
constexpr usize     MAP_SHARED      = 0x01;
// changes are private
constexpr usize     MAP_PRIVATE     = 0x02;
// mask for type of mapping
constexpr usize     MAP_TYPE        = 0x0f;
// interpret addr exactly
constexpr usize     MAP_FIXED       = 0x10;
// don't use a file
constexpr usize     MAP_ANONYMOUS   = 0x20;
// only give out 32bit addresses
constexpr usize     MAP_32BIT       = 0x40;
// stack-like segment
constexpr usize     MAP_GROWSDOWN   = 0x100;
constexpr usize     MAP_DENYWRITE   = 0x800;
// mark it as an executable
constexpr usize     MAP_EXECUTABLE  = 0x1000;
// pages are locked
constexpr usize     MAP_LOCKED      = 0x2000;
// don't check for reservations
constexpr usize     MAP_NORESERVE   = 0x4000;

constexpr usize     MAP_HUGE_SHIFT  = 26;
constexpr usize     MAP_HUGE_MASK   = 0x3f;

constexpr usize     MAP_HUGE_16KB   = (14 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_64KB   = (16 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_512KB  = (19 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_1MB    = (20 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_2MB    = (21 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_8MB    = (23 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_16MB   = (24 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_32MB   = (25 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_256MB  = (28 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_512MB  = (29 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_1GB    = (30 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_2GB    = (31 << MAP_HUGE_SHIFT);
constexpr usize     MAP_HUGE_16GB   = (34u << MAP_HUGE_SHIFT);

// sync memory asynchronously
constexpr usize     MS_ASYNC        = 0x01;
// invalidate the caches
constexpr usize     MS_INVALIDATE   = 0x02;
// synchronous memory sync
constexpr usize     MS_SYNC         = 0x04;

// default page-in behaviour
constexpr usize     MADV_NORMAL     = 0x00;
// page-in minimum required
constexpr usize     MADV_RANDOM     = 0x01;
// read-ahead aggressively
constexpr usize     MADV_SEQUENTIAL = 0x02;
// pre-fault pages
constexpr usize     MADV_WILLNEED   = 0x03;
// discard these pages
constexpr usize     MADV_DONTNEED   = 0x04;

// lock all current mappings
constexpr usize     MCL_CURRENT     = 1;
// lock all future mappings
constexpr usize     MCL_FUTURE      = 2;
