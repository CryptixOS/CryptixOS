/*
 * Created by v1tr10l7 on 15.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Color.hpp>
#include <Prism/Containers/Span.hpp>

enum class MemoryModel : u8
{
    eRGB = 1,
};

struct VideoMode
{
    u64              Pitch          = 0;
    u64              Width          = 0;
    u64              Height         = 0;
    u16              BitsPerPixel   = 0;
    enum MemoryModel MemoryModel    = MemoryModel::eRGB;
    u8               RedMaskSize    = 0;
    u8               RedMaskShift   = 0;
    u8               GreenMaskSize  = 0;
    u8               GreenMaskShift = 0;
    u8               BlueMaskSize   = 0;
    u8               BlueMaskShift  = 0;
};

struct Framebuffer
{
    Pointer                        Address        = nullptr;
    u64                            Width          = 0;
    u64                            Height         = 0;
    u64                            Pitch          = 0;
    u16                            BitsPerPixel   = 0;
    enum MemoryModel               MemoryModel    = MemoryModel::eRGB;
    u8                             RedMaskSize    = 0;
    u8                             RedMaskShift   = 0;
    u8                             GreenMaskSize  = 0;
    u8                             GreenMaskShift = 0;
    u8                             BlueMaskSize   = 0;
    u8                             BlueMaskShift  = 0;
    u64                            EDID_Size      = 0;
    Pointer                        EDID           = nullptr;
    Span<VideoMode, DynamicExtent> VideoModes;

    inline void                    PutPixel(usize x, usize y, Color color) const
    {
        u32   red                          = color.Red() << RedMaskShift;
        u32   green                        = color.Green() << GreenMaskShift;
        u32   blue                         = color.Blue() << BlueMaskShift;

        u32   rgb                          = red | green | blue;
        usize offset                       = y * (Pitch / sizeof(u32)) + x;
        Address.As<volatile u32>()[offset] = rgb;
    }
};
