/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

namespace KernelHeap
{
    void  Initialize();
    void* Allocate(usize bytes);
    void* Callocate(usize bytes);
    void* Reallocate(void* ptr, usize size);
    void  Free(void* memory);

    template <typename T>
    inline T Allocate(usize bytes)
    {
        return reinterpret_cast<T>(Allocate(bytes));
    }
    template <typename T>
    inline T Callocate(size_t bytes)
    {
        return reinterpret_cast<T>(Callocate(bytes));
    }
    template <typename T>
    T Reallocate(T ptr, size_t size)
    {
        return reinterpret_cast<T>(
            Reallocate(reinterpret_cast<void*>(ptr), size));
    }

    template <typename T>
    void Free(T memory)
    {
        return Free(reinterpret_cast<void*>(memory));
    }
} // namespace KernelHeap
