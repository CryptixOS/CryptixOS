/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Utility/Types.hpp"

#include <cstring>

struct Bitmap
{
    Bitmap() = default;
    Bitmap(u8* data, const usize size, const u8 value = 1)
        : data(data)
        , size(size)
    {
        SetAll(value);
    }

    inline void SetAll(const u8 value) { memset(data, value, size); }
    inline void SetIndex(const usize index, const bool value)
    {
        const usize byte = index / 8;
        const usize bit  = index % 8;
        Assert(byte < size);

        if (value) data[byte] |= Bit(bit);
        else data[byte] &= ~Bit(bit);
    }
    inline bool GetIndex(const usize index) const
    {
        const usize byte = index / 8;
        const usize bit  = index % 8;

        return data[byte] & Bit(bit);
    }

    u8*   data;
    usize size;
};
