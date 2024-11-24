/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Types.hpp"

namespace PhysicalMemoryManager
{
    inline constexpr const usize PAGE_SIZE = 0x1000;

    bool                         Initialize();
    bool                         IsInitialized();

    void*                        AllocatePages(usize count = 1);
    void*                        CallocatePages(usize count = 1);
    void                         FreePages(void* ptr, usize count);

    template <typename T>
    T AllocatePages(usize count = 1)
    {
        return reinterpret_cast<T>(AllocatePages(count));
    }
    template <typename T>
    T CallocatePages(usize count = 1)
    {
        return reinterpret_cast<T>(CallocatePages(count));
    }
    template <typename T>
    void FreePages(T ptr, usize count)
    {
        FreePages(reinterpret_cast<void*>(ptr), count);
    }

    uintptr_t GetMemoryTop();
    usize     GetTotalMemory();
    usize     GetFreeMemory();
    usize     GetUsedMemory();
}; // namespace PhysicalMemoryManager

namespace PMM = PhysicalMemoryManager;
