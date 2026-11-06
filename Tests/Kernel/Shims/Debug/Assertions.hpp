/*
 * Created by v1tr10l7 on 06.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

// Host stand-in for Kernel/Debug/Assertions.hpp, so kernel sources that only
// assert can be compiled into the host tests
#include <cassert>
#include <cstdlib>

#define Assert(expr)       assert(expr)
#define AssertNotReached() std::abort()
