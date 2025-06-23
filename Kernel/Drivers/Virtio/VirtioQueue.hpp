/*
 * Created by v1tr10l7 on 17.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

namespace Virtio
{
    struct [[gnu::packed]] Buffer
    {
        u64 Address;
        u32 Length;
        u16 Flags;
        u16 Next;
    };
    struct [[gnu::packed]] AvailableRing
    {
        u16 Flags;
        u16 Index;
        u16 Rings[];
    };

    class Queue
    {
      public:
    };
}; // namespace Virtio
