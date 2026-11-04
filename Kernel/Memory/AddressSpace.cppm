/*
 * Created by v1tr10l7 on 23.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Config.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:Memory.AddressSpace;

export import :Library.Locking.Spinlock;
export import :Memory.Region;

export class AddressSpace : public NonCopyable<AddressSpace>, public RefCounted
{
  public:
    AddressSpace();
    ~AddressSpace();

    void        Initialize(Pointer start, usize size);

    bool        IsAvailable(Pointer base, usize length) const;

    inline void Insert(::Ref<Region> region)
    {
        return Insert(region->VirtualBase(), region);
    }
    void          Insert(Pointer, ::Ref<Region> region);
    void          Erase(Pointer);

    ::Ref<Region> AllocateRegion(usize size, usize alignment = 0);
    ::Ref<Region> AllocateFixed(Pointer requestedAddress, usize size);

    ::Ref<Region> Find(Pointer virt) const;
    bool          Contains(Pointer virt) const { return Find(virt) != nullptr; }

    inline ::Ref<Region> operator[](Pointer virt) { return m_RegionTree[virt]; }
    void                 Clear();

    auto                 begin() { return m_RegionTree.begin(); }
    auto                 end() { return m_RegionTree.end(); }

    RedBlackTree<Pointer, ::Ref<Region>> m_RegionTree;

    void                                 Dump();

  private:
    Spinlock     m_Lock;

    AddressRange m_TotalRange;
};
