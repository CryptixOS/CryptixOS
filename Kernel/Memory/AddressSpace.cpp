/*
 * Created by v1tr10l7 on 17.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/AddressSpace.hpp>
#include <Memory/MM.hpp>
#include <Memory/PMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <Prism/Utility/Optional.hpp>

AddressSpace::AddressSpace()
// : m_TotalRange(USER_VIRTUAL_RANGE_BASE, USER_VIRTUAL_RANGE_SIZE)
{
    auto memoryTop = PMM::GetMemoryTop();
    auto base      = Pointer(Math::AlignUp(memoryTop, 1_gib));
    m_TotalRange   = {base.Offset(4_gib), MM::HigherHalfOffset()};
}
AddressSpace::~AddressSpace() { m_RegionTree.Clear(); }

void AddressSpace::Initialize(Pointer start, usize size)
{
    m_TotalRange = {start, start.Offset(size)};
}

bool AddressSpace::IsAvailable(Pointer base, usize length) const
{
    auto end = base.Offset(length);

    for (const auto& entry : m_RegionTree)
    {
        ::Ref current      = entry.Value;

        auto  currentStart = current->VirtualBase();
        auto  currentEnd   = current->End();

        if (end <= currentStart || base >= currentEnd) continue;
        return false;
    }

    return true;
}
void AddressSpace::Insert(Pointer base, ::Ref<Region> region)
{
    ScopedLock guard(m_Lock);
    m_RegionTree.Insert(base, region);
}
void AddressSpace::Erase(Pointer base)
{
    ScopedLock guard(m_Lock);
    auto       it = m_RegionTree.Find(base);

    if (it != m_RegionTree.end()) m_RegionTree.Erase(it->Key);
}

::Ref<Region> AddressSpace::AllocateRegion(usize length, usize alignment)
{
    if (!alignment) alignment = PMM::PAGE_SIZE;
    Pointer current = Math::AlignUp(m_TotalRange.Base(), alignment);

    for (auto it = m_RegionTree.begin(); it != m_RegionTree.end(); ++it)
    {
        auto region      = it->Value;
        auto regionStart = region->VirtualBase();

        // Align current pointer before checking
        current          = Math::AlignUp(current, alignment);

        // If aligned current + length fits before this region
        if (current.Offset(length) <= regionStart
            && IsAvailable(current, length))
        {
            ::Ref region = new Region(0, current, length);
            Insert(region->VirtualBase(), region);
            return region;
        }

        // Move current to the end of this region and keep going
        current = region->End();
    }

    // Check space after the last region
    current = Math::AlignUp(current, alignment);
    if (IsAvailable(current, length))
    {
        ::Ref region = new Region(0, current, length);
        Insert(region->VirtualBase(), region);
        return region;
    }

    return nullptr; // no suitable region found
}
::Ref<Region> AddressSpace::AllocateFixed(Pointer virt, usize size)
{
    size = Math::AlignUp(size, PMM::PAGE_SIZE);

    if (!IsAvailable(virt, size)) return nullptr;

    ::Ref<Region> region = new Region(0, virt, size);
    Insert(region->VirtualBase(), region);
    return region;
}

::Ref<Region> AddressSpace::Find(Pointer address) const
{
    for (const auto& entry : m_RegionTree)
    {
        ::Ref region = entry.Value;

        auto  size   = region->Size();
        auto  start  = region->VirtualBase();
        auto  end    = start.Offset(size);

        if (address >= start && address < end) return region;
    }

    return nullptr;
}

void AddressSpace::Clear() { m_RegionTree.Clear(); }

void AddressSpace::Dump()
{
    for (const auto& entry : m_RegionTree)
    {
        auto  virt   = entry.Key;
        ::Ref region = entry.Value;
        LogDebug("{:#x} => Region[{:#x}] => {{ base: {:#x}, size: {:#x} }}",
                 (upointer)region.Raw(), virt, region->VirtualBase().Raw(),
                 region->Size());
    }
}
