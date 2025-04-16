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
#include <Prism/Memory/Pointer.hpp>

class AddressSpace
{
  public:
    AddressSpace();
    ~AddressSpace();

    ErrorOr<Region*> AllocateRegion(Pointer requestedAddress, usize size);

    inline PageMap*  GetPageMap() const { return m_PageMap; }

  private:
    PageMap*                         m_PageMap = nullptr;
    RedBlackTree<uintptr_t, Region*> m_RegionTree;

    constexpr static Pointer         USERSPACE_VIRT_BASE{0x100000zu};

    AddressRange                     m_TotalRange
        = {USERSPACE_VIRT_BASE,
           BootInfo::GetHHDMOffset() - USERSPACE_VIRT_BASE.Raw() - 0x1000000zu};
};
