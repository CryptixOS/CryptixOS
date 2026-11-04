/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>

export module Cryptix:Memory.PMM;

export import :Boot.BootMemoryInfo;
export import :Debug.Assertions;
export import :Library.Logger;
export import :Library.Stacktrace;

export namespace PMM
{
    constexpr usize     PAGE_SIZE = 0x1000;

    CTOS_NO_KASAN bool  Initialize(const MemoryMap& memoryMap);
    bool                IsInitialized();

    Span<MemoryZone>    MemoryZones();

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
        if constexpr (IsSameV<T, Pointer>) return AllocatePages(count);

        return reinterpret_cast<T>(CallocatePages(count));
    }

    template <PointerHolder T>
    CTOS_NO_KASAN void FreePages(T ptr, usize count)
    {
        if constexpr (IsSameV<T, Pointer>)
            FreePages(ptr.template FromHigherHalf<void*>(), count);
        else FreePages(reinterpret_cast<void*>(ptr), count);
    }

    uintptr_t GetMemoryTop();
    usize     GetTotalMemory();
    usize     GetFreeMemory();
    usize     GetUsedMemory();
}; // namespace PMM
