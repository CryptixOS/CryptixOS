/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <compare>
#include <concepts>
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
    template <std::unsigned_integral T = uintptr_t>
    Pointer(T m_Pointer)
        : m_Pointer(m_Pointer)
    {
    }

    Pointer(void* m_Pointer)
        : m_Pointer(reinterpret_cast<uintptr_t>(m_Pointer))
    {
    }

    operator uintptr_t() { return m_Pointer; }
    operator void*() { return reinterpret_cast<void*>(m_Pointer); }
    operator bool() { return m_Pointer != 0; }
    Pointer& operator=(uintptr_t addr)
    {
        m_Pointer = addr;
        return *this;
    }
    Pointer& operator=(void* addr)
    {
        m_Pointer = reinterpret_cast<uintptr_t>(addr);
        return *this;
    }

    template <typename T>
    inline T* As() const
    {
        return reinterpret_cast<T*>(m_Pointer);
    }
    template <typename T = uintptr_t>
    T Raw()
    {
        return static_cast<T>(m_Pointer);
    }

    inline bool IsHigherHalf() const
    {
        return m_Pointer >= BootInfo::GetHHDMOffset();
    }

    template <typename T = uintptr_t>
        requires(std::is_pointer_v<T> || std::is_integral_v<T>
                 || std::is_same_v<T, Pointer>)
    inline T ToHigherHalf() const
    {
        return IsHigherHalf()
                 ? reinterpret_cast<T>(m_Pointer)
                 : reinterpret_cast<T>(m_Pointer + BootInfo::GetHHDMOffset());
    }
    template <>
    inline Pointer ToHigherHalf() const
    {
        return ToHigherHalf<uintptr_t>();
    }

    inline Pointer ToHigherHalf() const
    {
        return IsHigherHalf() ? m_Pointer
                              : m_Pointer + BootInfo::GetHHDMOffset();
    }

    template <typename T>
    inline T FromHigherHalf() const
    {
        return IsHigherHalf()
                 ? reinterpret_cast<T>(m_Pointer - BootInfo::GetHHDMOffset())
                 : reinterpret_cast<T>(m_Pointer);
    }

    template <typename T = uintptr_t>
    inline T Offset(uintptr_t offset) const
    {
        return reinterpret_cast<T>(m_Pointer + offset);
    }
    template <>
    inline Pointer Offset(uintptr_t offset) const
    {
        return m_Pointer + offset;
    }

    inline auto operator<=>(const Pointer& other) const = default;

  private:
    uintptr_t m_Pointer = 0;
};
