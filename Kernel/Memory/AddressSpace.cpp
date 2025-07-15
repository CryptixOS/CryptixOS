/*
 * Created by v1tr10l7 on 17.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/AddressSpace.hpp>
#include <Memory/MemoryManager.hpp>
#include <Memory/PMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <Prism/Utility/Optional.hpp>

AddressSpace::AddressSpace()
// : m_TotalRange(USER_VIRTUAL_RANGE_BASE, USER_VIRTUAL_RANGE_SIZE)
{
    auto memoryTop = PMM::GetMemoryTop();
    auto base      = Pointer(Math::AlignUp(memoryTop, 1_gib));
    m_TotalRange   = {base.Offset(4_gib), MemoryManager::HigherHalfOffset()};
}
AddressSpace::~AddressSpace() { m_RegionTree.Clear(); }

bool AddressSpace::IsAvailable(Pointer base, usize length) const
{
    auto end = base.Offset(length);

    for (const auto& [virt, current] : m_RegionTree)
    {
        auto currentStart = current->VirtualBase();
        auto currentEnd   = current->End();

        if (end < currentStart || currentEnd < base) return false;
    }

    return true;
}
void AddressSpace::Insert(Pointer base, Ref<Region> region)
{
    m_RegionTree.Insert(base.Raw(), region);
}
void AddressSpace::Erase(Pointer base)
{
    ScopedLock guard(m_Lock);
    auto       it = m_RegionTree.Find(base.Raw());

    if (it != m_RegionTree.end()) m_RegionTree.Erase(it->Key);
}

Ref<Region> AddressSpace::AllocateRegion(usize size, usize alignment)
{
    ScopedLock guard(m_Lock);
    if (alignment == 0) alignment = sizeof(void*);

    auto windowStart = m_TotalRange.Base();
    auto allocateFromWindow
        = [&](AddressRange const& window) -> Optional<AddressRange>
    {
        if (window.Size() < (size + alignment)) return NullOpt;

        auto initialBase = window.Base();
        auto alignedBase
            = Math::RoundUpToPowerOfTwo(initialBase.Raw(), alignment);

        Assert(size);
        return AddressRange(alignedBase, size);
    };

    Optional<AddressRange> found;
    for (const auto& [key, region] : m_RegionTree)
    {
        if (windowStart == region->VirtualBase())
        {
            windowStart = region->VirtualBase().Offset(region->Size());
            continue;
        }

        AddressRange window(windowStart,
                            region->VirtualBase().Raw() - windowStart.Raw());
        auto         maybeRange = allocateFromWindow(window);
        if (maybeRange) found = maybeRange.Value();
    }

    if (!found) return nullptr;

    auto        area        = found.Value();
    Pointer     regionStart = area.Base();
    usize       regionSize  = area.Size();

    Ref<Region> region      = new Region(0, regionStart, regionSize);

    Insert(region->VirtualBase().Raw(), region);
    return region;
}
Ref<Region> AddressSpace::AllocateFixed(Pointer virt, usize size)
{
    ScopedLock guard(m_Lock);
    size = Math::AlignUp(size, PMM::PAGE_SIZE);

    if (!IsAvailable(virt, size)) return nullptr;

    Ref<Region> region = new Region(0, virt, size);
    Insert(region->VirtualBase(), region);
    return region;
}

Ref<Region> AddressSpace::Find(Pointer address) const
{
    for (const auto& [virt, region] : m_RegionTree)
    {
        auto size  = region->Size();
        auto start = region->VirtualBase();
        auto end   = start.Offset(size);

        if (address >= start.Raw() && address < end) return region;
    }

    return nullptr;
}

void AddressSpace::Clear() { m_RegionTree.Clear(); }
