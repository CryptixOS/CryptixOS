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
    for (const auto& region : m_Regions) delete region;
    // for (const auto& [base, region] : m_RegionTree) delete region;
    m_Regions.Clear();
    // m_RegionTree.Clear();
}

bool AddressSpace::IsAvailable(Pointer base, usize length) const
{
    for (const auto* region : *const_cast<AddressSpace*>(this))
    {
        auto range = region->GetVirtualRange();
        if (range.Contains(base) || range.Contains(base.Offset(length - 1)))
            return false;
    }

    return true;
}
void AddressSpace::Insert(Pointer, Region* region)
{
    m_Regions.PushBack(region);
}
void AddressSpace::Erase(Pointer base)
{
    ScopedLock guard(m_Lock);
    auto       it = begin();
    for (it = begin(); it != end(); it++)
        if ((*it)->GetVirtualBase() == base) break;

    if (it != end())
    {
        delete *it;
        m_Regions.Erase(it);
    }
}

Region* AddressSpace::AllocateRegion(Pointer requestedAddress, usize size)
{
    ScopedLock guard(m_Lock);

    if (requestedAddress == nullptr)
        requestedAddress = VMM::AllocateSpace(size, 0);
    auto region = new Region(0, requestedAddress, size);

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
Region* AddressSpace::AllocateFixed(Pointer requestedAddress, usize size)
{
    ScopedLock guard(m_Lock);

    auto       virt = Math::AlignUp(requestedAddress, PMM::PAGE_SIZE);
    size            = Math::AlignUp(size, PMM::PAGE_SIZE);

    if (!IsAvailable(virt, size)) return nullptr;

    auto region = new Region(0, virt, size);
    Insert(region->GetVirtualBase(), region);
    return region;
}
