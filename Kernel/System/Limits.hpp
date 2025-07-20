/*
 * Created by v1tr10l7 on 09.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Memory.hpp>

namespace System::Limits
{
    inline constexpr usize PROCESS_CHILDREN = 64;
    inline constexpr usize PROCESS_ARGS     = 128_kib;
    inline constexpr usize GROUPS           = 64;
    inline constexpr usize OPEN_FILES       = 0x100000;
    inline constexpr usize MOUNTS           = 256;
    inline constexpr usize HARD_LINKS       = 127;
    inline constexpr usize FILE_NAME        = 255;
    inline constexpr usize PATH             = 4096;
    inline constexpr usize PIPE_BUFFER      = 4_kib;
    inline constexpr usize TTY_CANON_QUEUE  = 4_kib;
    inline constexpr usize TTY_INPUT_BUFFER = 4_kib;
    inline constexpr usize XATTR_NAME       = 255;
    inline constexpr usize XATTR_VALUE      = 64_kib;
    inline constexpr usize XATTR_NAME_LIST  = 64_kib;
}; // namespace System::Limits
