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
    auto base      = Pointer(Math::AlignUp(memoryTop, 1_gib)).Offset<Pointer>(4_gib);
    // The range is (base, size): it ends at the top of the user half.
    usize top      = USER_VIRTUAL_RANGE_TOP;
    m_TotalRange   = {base, base.Raw() < top ? top - base.Raw() : 0};
}
AddressSpace::~AddressSpace() { m_RegionTree.Clear(); }

void AddressSpace::Initialize(Pointer start, usize size)
{
    ScopedLock guard(m_Lock);
    m_TotalRange = {start, size};
}

bool AddressSpace::IsAvailable(Pointer base, usize length) const
{
    ScopedLock guard(m_Lock);
    return IsAvailableLocked(base, length);
}
bool AddressSpace::IsAvailableLocked(Pointer base, usize length) const
{
    auto end = base.Offset(length);
    if (end < base) return false;

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
    // Regions are mapped page by page, so they must never share a page.
    alignment = Max(alignment, PMM::PAGE_SIZE);
    length    = Math::AlignUp(Max(length, 1zu), PMM::PAGE_SIZE);

    // The whole search + insert must be atomic, or two CPUs could be handed
    // the same range.
    ScopedLock guard(m_Lock);

    Pointer    rangeEnd = m_TotalRange.End();
    Pointer    current  = Math::AlignUp(m_TotalRange.Base(), alignment);
    auto       fits     = [&](Pointer base) -> bool
    {
        Pointer end = base.Offset(length);
        return end > base && end <= rangeEnd && IsAvailableLocked(base, length);
    };
    auto create = [&](Pointer base) -> ::Ref<Region>
    {
        ::Ref region = CreateRef<Region>(nullptr, base, length);
        m_RegionTree.Insert(region->VirtualBase(), region);
        return region;
    };

    // The tree is ordered by base address: walk the gaps between regions.
    for (auto it = m_RegionTree.begin(); it != m_RegionTree.end(); ++it)
    {
        auto region = it->Value;

        // Regions that end before the cursor (e.g. below the range base)
        // must not move it backwards.
        if (region->End() <= current) continue;

        current = Math::AlignUp(current, alignment);
        if (current.Offset(length) <= region->VirtualBase() && fits(current))
            return create(current);

        current = Max(current, region->End());
    }

    // Check the space after the last region.
    current = Math::AlignUp(current, alignment);
    if (fits(current)) return create(current);

    return nullptr; // no suitable region found
}
::Ref<Region> AddressSpace::AllocateFixed(Pointer virt, usize size)
{
    size = Math::AlignUp(size, PMM::PAGE_SIZE);

    ScopedLock guard(m_Lock);
    if (!IsAvailableLocked(virt, size)) return nullptr;

    ::Ref<Region> region = CreateRef<Region>(nullptr, virt, size);
    m_RegionTree.Insert(region->VirtualBase(), region);
    return region;
}

::Ref<Region> AddressSpace::Find(Pointer address) const
{
    ScopedLock guard(m_Lock);
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

void AddressSpace::Clear()
{
    ScopedLock guard(m_Lock);
    m_RegionTree.Clear();
}

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
