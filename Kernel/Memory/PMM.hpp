/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>
#include <Boot/BootMemoryInfo.hpp>

#include <Prism/Containers/Span.hpp>
#include <Prism/Memory/Pointer.hpp>

namespace PMM
{
    constexpr usize     PAGE_SIZE = 0x1000;

    CTOS_NO_KASAN bool  Initialize(const MemoryMap& memoryMap);
    bool                IsInitialized();

    Span<MemoryZone>  MemoryZones();

    CTOS_NO_KASAN void* AllocatePages(usize count = 1);
    CTOS_NO_KASAN void* CallocatePages(usize count = 1);
    CTOS_NO_KASAN void  FreePages(void* ptr, usize count);

    template <PointerHolder T>
    inline CTOS_NO_KASAN T AllocatePages(usize count = 1)
    {
        if constexpr (IsSameV<T, Pointer>) return AllocatePages(count);

        return reinterpret_cast<T>(AllocatePages(count));
    }

    template <PointerHolder T>
    CTOS_NO_KASAN T CallocatePages(usize count = 1)
    {
        if constexpr (std::is_same_v<T, Pointer>) return AllocatePages(count);

        return reinterpret_cast<T>(CallocatePages(count));
    }

    template <PointerHolder T>
    CTOS_NO_KASAN void FreePages(T ptr, usize count)
    {
        if constexpr (std::is_same_v<T, Pointer>)
            FreePages(ptr.template FromHigherHalf<void*>(), count);
        else FreePages(reinterpret_cast<void*>(ptr), count);
    }

    uintptr_t GetMemoryTop();
    usize     GetTotalMemory();
    usize     GetFreeMemory();
    usize     GetUsedMemory();
}; // namespace PMM
