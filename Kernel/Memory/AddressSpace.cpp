/*
 * Created by v1tr10l7 on 17.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/AddressSpace.hpp>
#include <Prism/Utility/Optional.hpp>

AddressSpace::AddressSpace()
{
    auto memoryTop = PMM::GetMemoryTop();
    auto base      = Pointer(Math::AlignUp(memoryTop, 1_gib));
    m_TotalRange   = {base.Offset(4_gib), BootInfo::GetHHDMOffset()};
}
AddressSpace::~AddressSpace() { m_RegionTree.Clear(); }

bool AddressSpace::IsAvailable(Pointer base, usize length) const
{
#if 0
    auto end = base.Offset(length);

    for (const auto& [virt, region] : m_RegionTree)
    {
        auto regionStart = region->VirtualBase();
        auto regionEnd   = regionStart.Offset(region->Size());

        if (base < regionEnd && end > regionStart) return false;
    }

    return true;
#else
    return true;
    for (auto& [virt, region] : *const_cast<AddressSpace*>(this))
    {
        auto regionStart = region->VirtualBase();
        auto regionEnd   = regionStart.Offset(region->Size());

        if (base >= regionStart && base < regionEnd) return false;
    }

    return true;
#endif
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

#if 1
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

    auto    area        = found.Value();
    Pointer regionStart = area.Base();
    usize   regionSize  = area.Size();

#else
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
#endif

    Ref<Region> region = new Region(0, regionStart, regionSize);

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

        if (address.Raw() == 0x41800000)
        {
            LogDebug(
                "RegionTree: {:#x} => {{ .PhysBase: {:#x}, .VirtBase: "
                "{:#x}, .Size: {:#x}, .VirtEnd: {:#x} }}",
                virt, region->PhysicalBase().Raw(), region->VirtualBase().Raw(),
                region->Size(), region->VirtualBase().Offset(region->Size()));
        }

        if (address >= start.Raw() && address < end) return region;
        // else ("Check doesn't work");
    }

    return nullptr;
}

void AddressSpace::Clear() { m_RegionTree.Clear(); }
