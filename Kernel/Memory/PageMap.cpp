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
std::pair<usize, PageAttributes> PageMap::RequiredSize(usize size) const
{
    usize lPageSize  = Arch::VMM::GetPageSize(PageAttributes::eLPage);
    usize llPageSize = Arch::VMM::GetPageSize(PageAttributes::eLLPage);

    if (size >= llPageSize) return {llPageSize, PageAttributes::eLLPage};
    else if (size >= lPageSize) return {lPageSize, PageAttributes::eLPage};

    return {m_PageSize, static_cast<PageAttributes>(0)};
}

bool PageMap::ValidateAddress(Pointer address, PageAttributes flags)
{
    auto pte = Virt2Pte(m_TopLevel, address, false, PMM::PAGE_SIZE);
    if (!pte) return false;

    return Arch::VMM::FromNativeFlags(pte->Flags()) & flags;
}

ErrorOr<Pointer> PageMap::MapIoRegion(Pointer phys, usize length,
                                      PageAttributes flags, usize alignment)
{
    length    = Math::AlignUp(length, PMM::PAGE_SIZE);
    auto virt = VMM::AllocateSpace(length, alignment, true);
    phys      = Math::AlignDown(phys, PMM::PAGE_SIZE);

    if (MapRange(virt, phys, length, flags)) return virt;
    return Error(ENOMEM);
}

bool PageMap::Map(Pointer virt, Pointer phys, PageAttributes flags)
{
    u64 pte = Virt2Phys(virt);
    if (pte != static_cast<u64>(-1))
    {
        auto errorMessage = fmt::format(
            "VMM: Trying to map address {:#x} to {:#x}, but it is already "
            "mapped => {:#x}",
            virt.Raw(), phys.Raw(), pte);

        Assert(errorMessage.data());
    }

    // AssertFmt(entry == u64(-1),
    //           "VMM: Trying to map address {:#x} to {:#x}, but it is already "
    //           "mapped => {:#x}",
    //           virt.Raw(), phys.Raw(), entry);

    ScopedLock guard(m_Lock);

    return InternalMap(virt, phys, flags);
}
bool PageMap::Unmap(Pointer virt, PageAttributes flags)
{
    ScopedLock guard(m_Lock);
    return InternalUnmap(virt, flags);
}
bool PageMap::Remap(Pointer virtOld, Pointer virtNew, PageAttributes flags)
{
    Pointer phys = Virt2Phys(virtOld, flags);
    Unmap(virtOld, flags);

    return Map(virtNew, phys, flags);
}
bool PageMap::MapRange(Pointer virt, Pointer phys, usize size,
                       PageAttributes flags)
{
    usize pageSize = Arch::VMM::GetPageSize(flags);
    for (usize i = 0; i < size; i += pageSize)
    {
        if (!Map(virt.Offset(i), phys.Offset(i), flags))
        {
            UnmapRange(virt, i - pageSize);
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
    for (usize i = 0; i < size; i += pageSize)
        if (!Unmap(virt.Offset(i), flags)) return false;

    return true;
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
