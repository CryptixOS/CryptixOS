/*
 * Created by v1tr10l7 on 21.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

namespace Kasan
{
    enum class ShadowType : u8
    {
        eUnpoisoned8Bytes = 0,
        eUnpoisoned1Byte  = 1,
        eUnpoisoned2Byte  = 2,
        eUnpoisoned3Byte  = 3,
        eUnpoisoned4Byte  = 4,
        eUnpoisoned5Byte  = 5,
        eUnpoisoned6Byte  = 6,
        eUnpoisoned7Byte  = 7,
        eStackLeft        = 0xf1,
        eStackMiddle      = 0xf2,
        eStackRight       = 0xf3,
        eUseAfterReturn   = 0xf5,
        eUseAfterScope    = 0xf8,
        eGeneric          = 0xfa,
        eMalloc           = 0xfb,
        eFree             = 0xfc,
    };

    void Initialize(Pointer shadowBase);
    void FillShadow(Pointer address, usize size, ShadowType type);
    void MarkRegion(Pointer address, usize validSize, usize totalSize,
                    ShadowType type);
}; // namespace Kasan
