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
    for (auto& region : m_RegionTree) delete region;
    m_RegionTree.Clear();
}

ErrorOr<Region*> AddressSpace::AllocateRegion(Pointer requestedAddress,
                                              usize   size)
{
    auto virt = requestedAddress;
    if (virt.Raw() == 0) virt = m_TotalRange.GetBase();

    virt              = Math::AlignDown(virt, PMM::PAGE_SIZE);
    size              = Math::AlignUp(size, PMM::PAGE_SIZE);

    Region* newRegion = nullptr;
    for (auto& region : m_RegionTree)
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
    return newRegion;
}
