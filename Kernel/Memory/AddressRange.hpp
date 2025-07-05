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
    AddressRange() = default;

    AddressRange(Pointer address, usize size)
        : m_Base(address)
        , m_Size(size)
    {
    }

    inline Pointer Base() const { return m_Base; }
    inline usize   Size() const { return m_Size; }

    inline         operator bool() { return m_Base.operator bool(); }

    inline bool    Contains(Pointer address) const
    {
        return address >= m_Base && address < m_Base.Offset<Pointer>(m_Size);
    }

  private:
    Pointer m_Base = 0;
    usize   m_Size = 0;
};
