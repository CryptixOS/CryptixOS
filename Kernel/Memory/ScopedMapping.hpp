/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/VMM.hpp>

// Temporarily maps a physical structure of type T into kernel virtual memory
// for the lifetime of the object.
template <typename T>
class ScopedMapping
{
  public:
    ScopedMapping(Pointer phys, PageAttributes attributes)
        : m_PageAttributes(attributes)
    {
        // Cover every page the structure touches; `phys` need not be aligned.
        Pointer alignedPhys = Math::AlignDown(phys, PMM::PAGE_SIZE);
        usize   offset      = phys.Raw() - alignedPhys.Raw();
        m_MappingSize       = Math::AlignUp(offset + sizeof(T), PMM::PAGE_SIZE);

        m_Virt              = VMM::AllocateSpace(m_MappingSize, 0, false);
        if (!m_Virt) return;

        if (!VMM::MapKernelRange(m_Virt, alignedPhys, m_MappingSize,
                                 attributes))
        {
            VMM::FreeSpace(m_Virt);
            m_Virt = nullptr;
            return;
        }

        m_Structure = m_Virt.Offset<T*>(offset);
    }
    ~ScopedMapping()
    {
        if (!m_Structure) return;

        Assert(VMM::UnmapKernelRange(m_Virt, m_MappingSize, m_PageAttributes));
        VMM::FreeSpace(m_Virt);
    }

    ScopedMapping(const ScopedMapping&)            = delete;
    ScopedMapping& operator=(const ScopedMapping&) = delete;

    explicit       operator bool() const { return m_Structure; }
    T&             operator*() { return *m_Structure; }
    T*             operator->() { return m_Structure; }

  private:
    T*             m_Structure   = nullptr;
    Pointer        m_Virt        = nullptr;
    usize          m_MappingSize = 0;
    PageAttributes m_PageAttributes;
};
