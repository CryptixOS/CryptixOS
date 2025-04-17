/*
 * Created by v1tr10l7 on 23.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Boot/BootInfo.hpp>

#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Memory/Pointer.hpp>

class AddressSpace
{
  public:
    AddressSpace();
    ~AddressSpace();

    bool            IsAvailable(Pointer base, usize length) const;

    void            Insert(Pointer, Region* region);
    void            Erase(Pointer);

    Region*         AllocateRegion(Pointer requestedAddress, usize size);
    Region*         AllocateFixed(Pointer requestedAddress, usize size);

    auto            begin() { return m_Regions.begin(); }
    auto            end() { return m_Regions.end(); }

    // auto             begin() { return m_RegionTree.begin(); }
    // auto             end() { return m_RegionTree.end(); }

    inline PageMap* GetPageMap() const { return m_PageMap; }

    PageMap*        m_PageMap = nullptr;
    // RedBlackTree<uintptr_t, Region*> m_RegionTree;
    Vector<Region*> m_Regions;

  private:
    Spinlock                 m_Lock;
    constexpr static Pointer USERSPACE_VIRT_BASE{0x100000zu};

    AddressRange             m_TotalRange
        = {USERSPACE_VIRT_BASE,
           BootInfo::GetHHDMOffset() - USERSPACE_VIRT_BASE.Raw() - 0x1000000zu};
};
