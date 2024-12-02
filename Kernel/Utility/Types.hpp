/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <type_traits>

using usize  = size_t;
using isize  = std::make_signed_t<usize>;

using u8     = uint8_t;
using u16    = uint16_t;
using u32    = uint32_t;
using u64    = uint64_t;

using i8     = int8_t;
using i16    = int16_t;
using i32    = int32_t;
using i64    = int64_t;

using symbol = void*[];

inline constexpr u64 Bit(u64 n) { return (1ull << n); }

namespace BootInfo
{
    // We have to forward declare it here, to avoid recursive includes
    uintptr_t GetHHDMOffset();
}; // namespace BootInfo

struct Pointer
{
    Pointer(uintptr_t pointer)
        : pointer(pointer)
    {
    }
    Pointer(void* pointer)
        : pointer(reinterpret_cast<uintptr_t>(pointer))
    {
    }

    operator uintptr_t() { return pointer; }
    operator void*() { return reinterpret_cast<void*>(pointer); }
    operator bool() { return pointer != 0; }
    Pointer& operator=(uintptr_t addr)
    {
        pointer = addr;
        return *this;
    }
    Pointer& operator=(void* addr)
    {
        pointer = reinterpret_cast<uintptr_t>(addr);
        return *this;
    }

    template <typename T>
    T* As()
    {
        return reinterpret_cast<T*>(pointer);
    }

    inline bool IsHigherHalf() { return pointer >= BootInfo::GetHHDMOffset(); }

    template <typename T>
    inline T ToHigherHalf()
    {
        return IsHigherHalf()
                 ? reinterpret_cast<T>(pointer)
                 : reinterpret_cast<T>(pointer + BootInfo::GetHHDMOffset());
    }

    template <typename T>
    inline T FromHigherHalf()
    {
        return IsHigherHalf()
                 ? reinterpret_cast<T>(pointer - BootInfo::GetHHDMOffset())
                 : reinterpret_cast<T>(pointer);
    }

    uintptr_t pointer = 0;
};
