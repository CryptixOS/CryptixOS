/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Debug/Assertions.hpp>
#include <Prism/Types.hpp>

#include <cstring>

namespace Prism
{
    class Bitmap
    {
      public:
        Bitmap() = default;
        Bitmap(u8* data, const usize size, const u8 value = 1)
        {
            Initialize(data, size, value);
        }

        inline void Initialize(u8* data, const usize size, const u8 value = 1)
        {
            m_Data = data;
            m_Size = size;
            SetAll(value);
        }

        constexpr usize GetSize() const { return m_Size; }

        inline void     SetAll(const u8 value)
        {
            std::memset(m_Data, value, m_Size);
        }
        inline void SetIndex(const usize index, const bool value)
        {
            const usize byte = index / 8;
            const usize bit  = index % 8;
            PrismAssert(byte < m_Size);

            if (value) m_Data[byte] |= Bit(bit);
            else m_Data[byte] &= ~Bit(bit);
        }
        inline bool GetIndex(const usize index) const
        {
            const usize byte = index / 8;
            const usize bit  = index % 8;

            return m_Data[byte] & Bit(bit);
        }

      private:
        u8*   m_Data;
        usize m_Size;
    };
}; // namespace Prism
