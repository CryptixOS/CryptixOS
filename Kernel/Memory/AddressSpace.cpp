/*
 * Created by v1tr10l7 on 17.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/AddressSpace.hpp>

AddressSpace::AddressSpace() {}
AddressSpace::~AddressSpace()
{
    for (const auto& [base, region] : m_Regions) delete region;
    m_Regions.clear();

    // for (const auto& [base, region] : m_RegionTree) delete region;
    // m_RegionTree.Clear();
}

bool AddressSpace::IsAvailable(Pointer base, usize length) const
{
    return true;
    for (const auto& [base, region] : *const_cast<AddressSpace*>(this))
    {
        auto range = region->GetVirtualRange();
        if (range.Contains(base) /*|| range.Contains(base.Offset(length - 1))*/)
            return false;
    }

    return true;
}
void AddressSpace::Insert(Pointer base, Region* region)
{
    m_Regions[base.Raw()] = region;
}
void AddressSpace::Erase(Pointer base)
{
    ScopedLock guard(m_Lock);
    auto       it = m_Regions.find(base.Raw());

    if (it != m_Regions.end())
    {
        delete it->second;
        m_Regions.erase(it);
    }
}

Region* AddressSpace::AllocateRegion(usize size)
{
    ScopedLock guard(m_Lock);

    auto       virt   = VMM::AllocateSpace(size, 0, true);
    auto       region = new Region(0, virt, size);

    Insert(region->GetVirtualBase(), region);
    return region;

    /*
    auto virt = requestedAddress;
    if (virt.Raw() == 0) virt = m_TotalRange.GetBase();

    virt              = Math::AlignDown(virt, PMM::PAGE_SIZE);
    size              = Math::AlignUp(size, PMM::PAGE_SIZE);

    Region* newRegion = nullptr;
    for (const auto& [base, region] : m_RegionTree)
    {
        if (virt == region->GetVirtualBase())
        {
            virt = region->End();
            continue;
        }

        auto range = AddressRange(virt, region->GetVirtualBase() - virt);
        if (range.GetSize() < size)
        {
            virt = region->End();
            continue;
        }

        newRegion = new Region(0, virt, size);
        break;
    }

    if (!newRegion && !m_RegionTree.Contains(virt))
    {
        auto range = AddressRange(virt, size);
        newRegion  = new Region(0, range.GetBase(), range.GetSize());
    }

    if (newRegion)
        m_RegionTree.Insert(newRegion->GetVirtualBase().Raw(), newRegion);
    return newRegion;*/
}
Region* AddressSpace::AllocateFixed(Pointer virt, usize size)
{
    ScopedLock guard(m_Lock);
    size = Math::AlignUp(size, PMM::PAGE_SIZE);

    if (!IsAvailable(virt, size)) return nullptr;

    auto region = new Region(0, virt, size);
    Insert(region->GetVirtualBase(), region);
    return region;
}

void AddressSpace::Clear()
{
    for (const auto& [_, region] : *this) delete region;
    m_Regions.clear();
}
