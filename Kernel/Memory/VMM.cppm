/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>

export module Cryptix:Memory.VMM;

export import :Library.Locking.Spinlock;
export import :Memory.PMM;
export import :Memory.PageMap;
export import :Memory.Region;
import :Memory.AddressSpace;

export class AddressSpace;
export template <typename Address>
inline constexpr bool IsHigherHalfAddress(Address addr)
{
    using namespace MM;
    return reinterpret_cast<usize>(addr) >= HigherHalfOffset();
}

export template <typename T, typename U>
inline constexpr T ToHigherHalfAddress(U addr)
{
    using namespace MM;
    T ret = IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(reinterpret_cast<uintptr_t>(addr)
                                    + HigherHalfOffset());
    return ret;
}

export template <typename T, typename U>
inline constexpr T FromHigherHalfAddress(U addr)
{
    using namespace MM;
    T ret = !IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(reinterpret_cast<uintptr_t>(addr)
                                    - HigherHalfOffset());
    return ret;
}

export class PageMap;
export namespace Arch::VMM
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

export namespace VMM
{
    void          Initialize(Pointer kernelPhys, Pointer kernelVirt,
                             usize higherHalfOffset);
    void          UnmapKernelInitCode();

    usize         GetHigherHalfOffset();

    Pointer       AllocateSpace(usize increment = 0, usize alignment = 0,
                                bool lowerHalf = false);
    Ref<Region>   AllocateKernelRegion(usize count, PageAttributes flags);

    Ref<Region>   AllocateDMACoherent(usize          size,
                                      PageAttributes flags
                                      = PageAttributes::eRW
                                      | PageAttributes::eWriteThrough);
    void          FreeDMA_Region(Region& region);

    PageMap*      GetKernelPageMap();
    AddressSpace* GetKernelAddressSpace();

    void          SaveCurrentPageMap(PageMap& out);
    void          LoadPageMap(PageMap& pageMap, bool);

    bool    MapKernelRegion(Pointer virt, Pointer phys, usize pageCount = 1,
                            PageAttributes attributes
                            = PageAttributes::eWriteBack | PageAttributes::eRW);

    bool    MapKernelRange(Pointer virt, Pointer phys, usize size,
                           PageAttributes attributes
                           = PageAttributes::eWriteBack | PageAttributes::eRW);

    bool    UnmapKernelRange(Pointer virt, usize size,
                             PageAttributes flags
                             = static_cast<PageAttributes>(0));

    Pointer MapIoRegion(PhysAddr phys, usize size, bool write,
                        usize alignment = 0);
    template <typename T>
    inline T* MapIoRegion(PhysAddr phys, bool write = true,
                          usize alignment = alignof(T))
    {
        return MapIoRegion(phys, sizeof(T) + alignment, write, alignment)
            .As<T>();
    }
} // namespace VMM
