/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Module.hpp>
#include <Prism/Core/Types.hpp>

#ifdef CTOS_TARGET_X86_64
constexpr usize MAX_IO_PORT_COUNT = 65536;

struct IoPortResource
{
    u16     Base   = 0;
    u16     Length = 0;
    Module* Owner  = nullptr;

    friend class IntrusiveList<IoPortResource>;
    friend struct IntrusiveListHook<IoPortResource>;

    IntrusiveListHook<IoPortResource> Hook;
};
#endif
