/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/PageMap.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>

namespace Arch::VMM
{
    extern usize GetPageSize(PageAttributes flags);
};

// NOTE: m_Lock is always taken with interrupts disabled: page faults and
// interrupt handlers map memory too, which would deadlock on the same CPU.

PageMap::PageMap(Pointer topLevel)
    : m_TopLevel(topLevel.As<PageTable>())
{
}

void PageMap::operator=(Pointer topLevel)
{
    m_TopLevel = topLevel.As<PageTable>();
}

PageAttributes PageMap::PageSizeFlags(usize pageSize) const
{
    if (pageSize == Arch::VMM::GetPageSize(PageAttributes::eLPage))
        return PageAttributes::eLPage;
    if (pageSize == Arch::VMM::GetPageSize(PageAttributes::eLLPage))
        return PageAttributes::eLLPage;

    return static_cast<PageAttributes>(0);
}
KeyValuePair<usize, PageAttributes> PageMap::RequiredSize(usize size) const
{
    usize lPageSize  = Arch::VMM::GetPageSize(PageAttributes::eLPage);
    usize llPageSize = Arch::VMM::GetPageSize(PageAttributes::eLLPage);

    if (size >= llPageSize) return {llPageSize, PageAttributes::eLLPage};
    else if (size >= lPageSize) return {lPageSize, PageAttributes::eLPage};

    return {PMM::PAGE_SIZE, static_cast<PageAttributes>(0)};
}

bool PageMap::ValidateAddress(Pointer address, PageAttributes flags)
{
    auto pte = Virt2Pte(m_TopLevel, address, false, PMM::PAGE_SIZE);
    if (!pte || !pte->IsValid()) return false;

    return Arch::VMM::FromNativeFlags(pte->Flags()) & flags;
}

ErrorOr<Pointer> PageMap::MapIoRegion(Pointer phys, usize length,
                                      PageAttributes flags, usize alignment)
{
    // Map whole pages, but hand back the address of `phys` itself: dropping
    // its offset into the first page would return the wrong bytes.
    Pointer alignedPhys = Math::AlignDown(phys, PMM::PAGE_SIZE);
    usize   offset      = (phys - alignedPhys).Raw();
    length              = Math::AlignUp(length + offset, PMM::PAGE_SIZE);

    auto virt           = VMM::AllocateSpace(length, alignment, true);
    if (!virt) return Error(ENOMEM);

    if (MapRange(virt, alignedPhys, length, flags)) return virt.Offset(offset);
    return Error(ENOMEM);
}

bool PageMap::Map(Pointer virt, Pointer phys, PageAttributes flags)
{
    // InternalMap() itself detects (and reports) already mapped addresses.
    ScopedLock guard(m_Lock, true);
    return InternalMap(virt, phys, flags);
}
bool PageMap::Unmap(Pointer virt, PageAttributes flags)
{
    bool ok;
    {
        ScopedLock guard(m_Lock, true);
        ok = InternalUnmap(virt, flags);
    }
    Arch::VMM::ShootdownTlb();
    return ok;
}
bool PageMap::Remap(Pointer virtOld, Pointer virtNew, PageAttributes flags)
{
    Pointer phys = Virt2Phys(virtOld, flags);
    // Virt2Phys() reports failure with -1, not with nullptr.
    if (phys == Pointer(u64(-1))) return false;

    Unmap(virtOld, flags);
    return Map(virtNew, phys, flags);
}
bool PageMap::Protect(Pointer virt, PageAttributes flags)
{
    bool ok;
    {
        ScopedLock guard(m_Lock, true);
        ok = InternalProtect(virt, flags);
    }
    Arch::VMM::ShootdownTlb();
    return ok;
}

bool PageMap::MapRange(Pointer virt, Pointer phys, usize size,
                       PageAttributes flags)
{
    usize pageSize = Arch::VMM::GetPageSize(flags);

    // Page table entries can only map whole, aligned pages: cover every page
    // that [virt, virt + size) touches, with phys shifted along.
    Pointer alignedVirt = Math::AlignDown(virt, pageSize);
    usize   misalign    = (virt - alignedVirt).Raw();
    Pointer alignedPhys = Math::AlignDown(phys.Raw() - misalign, pageSize);
    usize   mapSize     = Math::AlignUp(size + misalign, pageSize);

    for (usize i = 0; i < mapSize; i += pageSize)
    {
        if (!Map(alignedVirt.Offset(i), alignedPhys.Offset(i), flags))
        {
            // Roll back everything mapped so far, at the same granularity.
            if (i) UnmapRange(alignedVirt, i, flags);
            return false;
        }
    }
    return true;
}
bool PageMap::RemapRange(Pointer virtOld, Pointer virtNew, usize size,
                         PageAttributes flags)
{
    usize pageSize = Arch::VMM::GetPageSize(flags);
    for (usize i = 0; i < size; i += pageSize)
        if (!Remap(virtOld.Offset(i), virtNew.Offset(i), flags)) return false;

    return true;
}
bool PageMap::UnmapRange(Pointer virt, usize size, PageAttributes flags)
{
    usize pageSize = Arch::VMM::GetPageSize(flags);
    bool  ok       = true;
    {
        ScopedLock guard(m_Lock, true);
        // Keep going past holes, so one unmapped page doesn't leave the rest
        // of the range mapped.
        for (usize i = 0; i < size; i += pageSize)
            ok &= InternalUnmap(virt.Offset(i), flags);
    }

    Arch::VMM::ShootdownTlb();
    return ok;
}

bool PageMap::ProtectRange(Pointer virt, usize size, PageAttributes flags)
{
    usize pageSize = Arch::VMM::GetPageSize(flags);
    bool  ok       = true;
    {
        ScopedLock guard(m_Lock, true);
        for (usize i = 0; i < size && ok; i += pageSize)
            ok = InternalProtect(virt.Offset(i), flags);
    }

    Arch::VMM::ShootdownTlb();
    return ok;
}

bool PageMap::MapRegion(const Ref<Region> region, const usize pageSize)
{
    const auto           virt = region->VirtualBase();
    const auto           phys = region->PhysicalBase();
    const usize          size = region->Size();

    const PageAttributes flags
        = region->PageAttributes() | PageSizeFlags(pageSize);
    return MapRange(virt, phys, size, flags);
}
bool PageMap::RemapRegion(const Ref<Region> region, Pointer newVirt)
{
    Pointer              oldVirt = region->VirtualBase();
    const usize          size    = region->Size();
    const PageAttributes flags   = region->PageAttributes();

    if (!newVirt) newVirt = region->VirtualBase();
    return RemapRange(oldVirt, newVirt, size, flags);
}

bool PageMap::SetFlagsRange(Pointer virt, usize size, PageAttributes flags)
{
    usize pageSize = Arch::VMM::GetPageSize(flags);
    for (usize i = 0; i < size; i += pageSize)
        if (!SetFlags(virt.Offset(i), flags)) return false;

    return true;
}
