/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <type_traits>

using usize  = size_t;
using isize  = std::make_signed_t<usize>;

using u8     = uint8_t;
using u16    = uint16_t;
using u32    = uint32_t;
using u64    = uint64_t;

using i8     = int8_t;
using i16    = int16_t;
using i32    = int32_t;
using i64    = int64_t;

using symbol = void*[];
