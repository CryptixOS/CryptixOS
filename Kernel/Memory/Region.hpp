/*
 * Created by v1tr10l7 on 18.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Memory/AddressRange.hpp>

namespace VirtualMemoryManager
{
    class Region final
    {
      public:
        Region() = default;
        Region(Pointer phys, Pointer virt, usize size)
            : m_VirtualSpace(virt, size)
            , m_PhysicalBase(phys)
        {
        }

        inline const AddressRange& GetVirtualRange() const
        {
            return m_VirtualSpace;
        }
        inline Pointer GetPhysicalBase() const { return m_PhysicalBase; }
        inline Pointer GetVirtualBase() const
        {
            return m_VirtualSpace.GetBase();
        }
        inline usize GetSize() const { return m_VirtualSpace.GetSize(); }

      private:
        AddressRange m_VirtualSpace;
        Pointer      m_PhysicalBase = nullptr;
    };

}; // namespace VirtualMemoryManager
