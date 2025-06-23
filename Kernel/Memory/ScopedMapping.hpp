/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/VMM.hpp>

template <typename T>
class ScopedMapping
{
  public:
    constexpr ScopedMapping(Pointer phys, PageAttributes attributes)
    {
        m_MappingSize    = Math::DivRoundUp(sizeof(T), PMM::PAGE_SIZE);
        m_PageAttributes = attributes;

        m_Structure      = Map(phys, phys, m_MappingSize, attributes);
    }
    constexpr ~ScopedMapping()
    {
        Assert(VMM::UnmapKernelRange(m_Structure, m_MappingSize,
                                     m_PageAttributes));
    }

    constexpr T& operator*() { return *m_Structure; }
    constexpr T* operator->() { return m_Structure; }

  private:
    T*             m_Structure   = nullptr;
    usize          m_MappingSize = 0;
    PageAttributes m_PageAttributes;

    constexpr T*   Map(Pointer virt, Pointer phys, usize size,
                       PageAttributes attributes)
    {
        if (!VMM::MapKernelRange(virt, phys, size, attributes)) return nullptr;

        return virt.As<T>();
    }
};
