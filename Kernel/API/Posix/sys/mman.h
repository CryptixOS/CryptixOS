/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

constexpr usize PROT_READ     = 0x01;
constexpr usize PROT_WRITE    = 0x02;
constexpr usize PROT_EXEC     = 0x04;

constexpr usize MAP_ANONYMOUS = 0x20;
