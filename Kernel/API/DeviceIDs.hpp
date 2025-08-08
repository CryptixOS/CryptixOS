/*
 * Created by v1tr10l7 on 08.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

namespace API
{
    namespace DeviceMajor
    {
        constexpr usize UNNAMED        = 0;
        constexpr usize MEMORY         = 1;
        constexpr usize RAMDISK        = 1;
        constexpr usize FLOPPY         = 2;
        constexpr usize PTYMASTER      = 2;
        constexpr usize PTYSLAVE       = 3;
        constexpr usize TTY            = 4;
        constexpr usize TTYAUX         = 5;
        constexpr usize LOOP           = 7;
        constexpr usize MISCELLANEOUS  = 10;
        constexpr usize INPUT          = 13;
        constexpr usize SOUND          = 14;
        constexpr usize FRAMEBUFFER    = 29;
        constexpr usize MSR            = 202;
        constexpr usize CPUID          = 203;
        constexpr usize BLOCK_EXTENDED = 259;
    }; // namespace DeviceMajor
}; // namespace API
