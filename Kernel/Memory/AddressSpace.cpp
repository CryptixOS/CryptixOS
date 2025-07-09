/*
 * Created by v1tr10l7 on 17.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/AddressSpace.hpp>

AddressSpace::AddressSpace()
{
    auto memoryTop = PMM::GetMemoryTop();
    auto base      = Pointer(Math::AlignUp(memoryTop, 1_gib));
    m_TotalRange   = {base.Offset(4_gib), BootInfo::GetHHDMOffset()};
}
AddressSpace::~AddressSpace() { m_RegionTree.Clear(); }

bool AddressSpace::IsAvailable(Pointer base, usize length) const
{
    return true;
    for (auto& [base, region] : *const_cast<AddressSpace*>(this))
    {
        auto range = region->VirtualRange();
        if (range.Contains(base) /*|| range.Contains(base.Offset(length - 1))*/)
            return false;
    }

    return true;
}
void AddressSpace::Insert(Pointer base, Ref<Region> region)
{
    m_RegionTree[base.Raw()] = region;
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

    Pointer currentRegion = m_TotalRange.Base();
    Pointer regionStart;
    usize   regionSize;
    Pointer regionEnd;

    do {
        regionStart = Math::AlignUp(currentRegion, alignment);
        regionSize  = (regionStart - currentRegion).Offset(size);
        currentRegion += regionSize;
        regionEnd = regionStart.Offset(regionSize);
    } while (m_RegionTree.Contains(regionStart.Raw())
             || m_RegionTree.Contains(regionEnd.Raw()));

    Ref<Region> region = new Region(0, regionStart, regionEnd - regionStart);

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

Ref<Region> AddressSpace::Find(Pointer virt)
{
    auto it = m_RegionTree.Find(virt);
    if (!it) return nullptr;

    return it->Value;
}

void AddressSpace::Clear() { m_RegionTree.Clear(); }
