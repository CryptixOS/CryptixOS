/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Spinlock.hpp>

#include <Memory/PMM.hpp>
#include <Memory/PageMap.hpp>
#include <Memory/Region.hpp>

#include <Prism/Utility/Math.hpp>

template <typename Address>
inline static constexpr bool IsHigherHalfAddress(Address addr)
{
    return reinterpret_cast<uintptr_t>(addr) >= BootInfo::GetHHDMOffset();
}

template <typename T, typename U>
inline static constexpr T ToHigherHalfAddress(U addr)
{
    T ret = IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(reinterpret_cast<uintptr_t>(addr)
                                    + BootInfo::GetHHDMOffset());
    return ret;
}

template <typename T, typename U>
inline static constexpr T FromHigherHalfAddress(U addr)
{
    T ret = !IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(reinterpret_cast<uintptr_t>(addr)
                                    - BootInfo::GetHHDMOffset());
    return ret;
}

class PageMap;
namespace Arch::VMM
{
    extern u64            g_DefaultPteFlags;

    extern void           Initialize();

    extern void*          AllocatePageTable();
    void                  DestroyPageMap(PageMap* pageMap);

    extern PageAttributes FromNativeFlags(usize flags);
    extern usize          ToNativeFlags(PageAttributes flags);

    extern usize          GetPageSize(PageAttributes flags
                                      = static_cast<PageAttributes>(0));
}; // namespace Arch::VMM

namespace VMM
{
    void        Initialize(Pointer kernelPhys, Pointer kernelVirt,
                           usize higherHalfOffset);
    void        UnmapKernelInitCode();

    usize       GetHigherHalfOffset();

    Pointer     AllocateSpace(usize increment = 0, usize alignment = 0,
                              bool lowerHalf = false);
    Ref<Region> AllocateDMACoherent(usize          size,
                                    PageAttributes flags
                                    = PageAttributes::eRW
                                    | PageAttributes::eWriteThrough);
    void        FreeDMA_Region(Region& region);

    PageMap*    GetKernelPageMap();

    void        SaveCurrentPageMap(PageMap& out);
    void        LoadPageMap(PageMap& pageMap, bool);

    bool        MapKernelRegion(Pointer virt, Pointer phys, usize pageCount = 1,
                                PageAttributes attributes
                                = PageAttributes::eWriteBack | PageAttributes::eRW);

    bool        MapKernelRange(Pointer virt, Pointer phys, usize size,
                               PageAttributes attributes
                               = PageAttributes::eWriteBack | PageAttributes::eRW);

    bool        UnmapKernelRange(Pointer virt, usize size,
                                 PageAttributes flags
                                 = static_cast<PageAttributes>(0));

    Pointer     MapIoRegion(PhysAddr phys, usize size, bool write,
                            usize alignment = 0);
    template <typename T>
    inline T* MapIoRegion(PhysAddr phys, bool write = true,
                          usize alignment = alignof(T))
    {
        return MapIoRegion(phys, sizeof(T) + alignment, write, alignment)
            .As<T>();
    }
} // namespace VMM
