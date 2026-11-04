/*
 * Created by v1tr10l7 on 12.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

constexpr usize ARCH_SET_GS               = 0x1001;
constexpr usize ARCH_SET_FS               = 0x1002;
constexpr usize ARCH_GET_FS               = 0x1003;
constexpr usize ARCH_GET_GS               = 0x1004;

constexpr usize ARCH_GET_CPUID            = 0x1011;
constexpr usize ARCH_SET_CPUID            = 0x1012;

constexpr usize ARCH_GET_XCOMP_SUPP       = 0x1021;
constexpr usize ARCH_GET_XCOMP_PERM       = 0x1022;
constexpr usize ARCH_REQ_XCOMP_PERM       = 0x1023;
constexpr usize ARCH_GET_XCOMP_GUEST_PERM = 0x1024;
constexpr usize ARCH_REQ_XCOMP_GUEST_PERM = 0x1025;

constexpr usize ARCH_XCOMP_TILECFG        = 17;
constexpr usize ARCH_XCOMP_TILEDATA       = 18;

constexpr usize ARCH_MAP_VDSO_X32         = 0x2001;
constexpr usize ARCH_MAP_VDSO_32          = 0x2002;
constexpr usize ARCH_MAP_VDSO_64          = 0x2003;

/* Don't use 0x3001-0x3004 because of old glibcs */
constexpr usize ARCH_GET_UNTAG_MASK       = 0x4001;
constexpr usize ARCH_ENABLE_TAGGED_ADDR   = 0x4002;
constexpr usize ARCH_GET_MAX_TAG_BITS     = 0x4003;
constexpr usize ARCH_FORCE_TAGGED_SVA     = 0x4004;

constexpr usize ARCH_SHSTK_ENABLE         = 0x5001;
constexpr usize ARCH_SHSTK_DISABLE        = 0x5002;
constexpr usize ARCH_SHSTK_LOCK           = 0x5003;
constexpr usize ARCH_SHSTK_UNLOCK         = 0x5004;
constexpr usize ARCH_SHSTK_STATUS         = 0x5005;

/* ARCH_SHSTK_ features bits */
constexpr usize ARCH_SHSTK_SHSTK          = Bit(0);
constexpr usize ARCH_SHSTK_WRSS           = Bit(1);
