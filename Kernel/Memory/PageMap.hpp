/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Spinlock.hpp>

#include <Memory/PMM.hpp>
#include <Memory/PageTableEntry.hpp>
#include <Memory/Region.hpp>

class PageMap;
namespace VMM
{
    Pointer AllocateSpace(usize increment, usize alignment, bool lowerHalf);
    void    LoadPageMap(PageMap& pageMap, bool);
}; // namespace VMM

class PageMap
{
  public:
    PageMap();
    PageMap(Pointer topLevel);

    void                             operator=(Pointer topLevel);

    [[nodiscard]] inline bool        Exists() const { return m_TopLevel; }
    [[nodiscard]] inline PageTable*  TopLevel() const { return m_TopLevel; }

    inline PageAttributes            PageSizeFlags(usize pageSize) const;
    std::pair<usize, PageAttributes> RequiredSize(usize size) const;

    void* NextLevel(PageTableEntry& entry, bool allocate, uintptr_t virt = -1);

    PageTableEntry*  Virt2Pte(PageTable* topLevel, Pointer virt, bool allocate,
                              u64 pageSize);
    Pointer          Virt2Phys(Pointer        virt,
                               PageAttributes flags = PageAttributes::eRead);

    bool             InternalMap(Pointer virt, Pointer phys,
                                 PageAttributes flags
                                 = PageAttributes::eRW | PageAttributes::eWriteBack);

    bool             InternalUnmap(Pointer        virt,
                                   PageAttributes flags = static_cast<PageAttributes>(0));

    ErrorOr<Pointer> MapIoRegion(Pointer phys, usize length,
                                 PageAttributes flags
                                 = PageAttributes::eRW
                                 | PageAttributes::eWriteThrough,
                                 usize alignment = 0);
    template <typename T>
    inline ErrorOr<T*> MapIoRegion(Pointer phys)
    {
        auto maybeVirt = MapIoRegion(phys, sizeof(T));
        if (!maybeVirt)
            return Error(maybeVirt.error());
        return maybeVirt.value();
    }

    bool        Map(Pointer virt, Pointer phys,
                    PageAttributes flags
                    = PageAttributes::eRW | PageAttributes::eWriteBack);
    bool        Unmap(Pointer        virt,
                      PageAttributes flags = static_cast<PageAttributes>(0));
    bool        Remap(Pointer virtOld, Pointer virtNew,
                      PageAttributes flags
                      = PageAttributes::eRW | PageAttributes::eWriteBack);

    bool        MapRange(Pointer virt, Pointer phys, usize size,
                         PageAttributes flags
                         = PageAttributes::eRW | PageAttributes::eWriteBack);
    bool        RemapRange(Pointer virtOld, Pointer virtNew, usize size,
                           PageAttributes flags
                           = PageAttributes::eRW | PageAttributes::eWriteBack);
    bool        UnmapRange(Pointer virt, usize size,
                           PageAttributes flags = static_cast<PageAttributes>(0));

    bool        MapRegion(const Ref<Region> region,
                          const usize       pageSize = PMM::PAGE_SIZE);
    bool        RemapRegion(const Ref<Region> region, Pointer newVirt = 0);

    bool        SetFlagsRange(Pointer virt, usize size,
                              PageAttributes flags
                              = PageAttributes::eRW | PageAttributes::eWriteBack);
    bool        SetFlags(Pointer        virt,
                         PageAttributes flags
                         = PageAttributes::eRW | PageAttributes::eWriteBack);

    inline void Load() { VMM::LoadPageMap(*this, true); }

  private:
    PageTable* m_TopLevel = 0;
    Spinlock   m_Lock;

    usize      m_PageSize = 0;
};
