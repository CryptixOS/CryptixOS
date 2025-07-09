/*
 * Created by v1tr10l7 on 09.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

namespace System::Limits
{
    inline constexpr usize OPEN_FILES = 0x100000;
    inline constexpr usize MOUNTS     = 256;
}; // namespace System::Limits
