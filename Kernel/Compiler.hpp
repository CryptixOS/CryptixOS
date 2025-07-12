/*
 * Created by v1tr10l7 on 12.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#define SECTION(name, ...)       [[gnu::section(name) __VA_OPT__(, ) __VA_ARGS__]]
#define ALWAYS_INLINE            [[always_inline]]
#define UNUSED                   [[maybe_unused]]
#define FORCE_EMIT               [[gnu::used]]
#define LIKELY                   [[likely]]
#define UNLIKELY                 [[unlikely]]

//--------------------------------------------------------------------------
// Sections
//--------------------------------------------------------------------------

#define KERNEL_INIT_SECTION_NAME ".kernel_init"
#define KERNEL_INIT_SECTION      SECTION(KERNEL_INIT_SECTION_NAME, FORCE_EMIT)

//--------------------------------------------------------------------------
// Compiler builtins
//--------------------------------------------------------------------------

// NOTE(v1tr10l7): index must be a value between 0 - 63
#define CtFrameAddress(index)    __builtin_frame_address(index)
#define CtCurrentFrameAddress()  CtFrameAddress(0)
