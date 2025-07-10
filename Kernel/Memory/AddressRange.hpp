/*
 * Created by v1tr10l7 on 17.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

class AddressRange final
{
  public:
    constexpr AddressRange() = default;

    constexpr AddressRange(Pointer address, usize size)
        : m_Base(address)
        , m_Size(size)
    {
    }

    constexpr inline Pointer Base() const { return m_Base; }
    constexpr inline Pointer End() const { return Base().Offset(Size()); }
    constexpr inline usize   Size() const { return m_Size; }

    constexpr inline         operator bool() { return m_Base.operator bool(); }

    constexpr inline bool    Contains(Pointer address) const
    {
        return address >= m_Base && address < m_Base.Offset<Pointer>(m_Size);
    }

    bool Intersects(AddressRange const& other) const
    {
        auto a = *this;
        auto b = other;

        if (a.Base() > b.Base()) std::swap(a, b);

        return a.Base() < b.End() && b.Base() < a.End();
    }

  private:
    Pointer m_Base = 0;
    usize   m_Size = 0;
};
