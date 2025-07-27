/*
 * Created by v1tr10l7 on 23.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Config.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Memory/Region.hpp>

#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Memory/WeakRef.hpp>

class AddressSpace : public NonCopyable<AddressSpace>
{
  public:
    AddressSpace();
    ~AddressSpace();

    bool        IsAvailable(Pointer base, usize length) const;

    void        Insert(Pointer, Ref<Region> region);
    void        Erase(Pointer);

    Ref<Region> AllocateRegion(usize size, usize alignment = 0);
    Ref<Region> AllocateFixed(Pointer requestedAddress, usize size);

    Ref<Region> Find(Pointer virt) const;
    bool        Contains(Pointer virt) const { return Find(virt) != nullptr; }

    inline Ref<Region> operator[](Pointer virt) { return m_RegionTree[virt]; }
    void               Clear();

    auto               begin() { return m_RegionTree.begin(); }
    auto               end() { return m_RegionTree.end(); }

    RedBlackTree<Pointer, Ref<Region>> m_RegionTree;

    void                               Dump();

  private:
    Spinlock     m_Lock;

    AddressRange m_TotalRange;
};
