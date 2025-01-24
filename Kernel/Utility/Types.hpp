/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <cerrno>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <type_traits>

using usize     = std::size_t;
using isize     = std::make_signed_t<usize>;

using u8        = std::uint8_t;
using u16       = std::uint16_t;
using u32       = std::uint32_t;
using u64       = std::uint64_t;

using i8        = std::int8_t;
using i16       = std::int16_t;
using i32       = std::int32_t;
using i64       = std::int64_t;

using symbol    = void*[];
using Error     = std::errno_t;
using ErrorType = std::errno_t;

template <typename R>
using ErrorOr = std::expected<R, ErrorType>;

inline constexpr u64 Bit(u64 n) { return (1ull << n); }

namespace BootInfo
{
    // We have to forward declare it here, to avoid recursive includes
    uintptr_t GetHHDMOffset();
}; // namespace BootInfo

struct Pointer
{
    template <std::unsigned_integral T = std::uintptr_t>
    Pointer(const T m_Pointer)
        : m_Pointer(m_Pointer)
    {
    }

    Pointer(const void* m_Pointer)
        : m_Pointer(reinterpret_cast<std::uintptr_t>(m_Pointer))
    {
    }

    operator std::uintptr_t() { return m_Pointer; }
    operator void*() { return reinterpret_cast<void*>(m_Pointer); }
    operator bool() { return m_Pointer != 0; }
    Pointer& operator=(std::uintptr_t addr)
    {
        m_Pointer = addr;
        return *this;
    }
    Pointer& operator=(void* addr)
    {
        m_Pointer = reinterpret_cast<std::uintptr_t>(addr);
        return *this;
    }

    template <typename T>
    inline T* As() const
    {
        return reinterpret_cast<T*>(m_Pointer);
    }
    template <typename T = std::uintptr_t>
    T Raw()
    {
        return static_cast<T>(m_Pointer);
    }

    auto&       operator->() { return *As<u64>(); }
    auto&       operator*() { return *As<u64>(); }

    inline bool IsHigherHalf() const
    {
        return m_Pointer >= BootInfo::GetHHDMOffset();
    }

    template <typename T = std::uintptr_t>
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
        return ToHigherHalf<std::uintptr_t>();
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

    template <typename T = std::uintptr_t>
    inline T Offset(std::uintptr_t offset) const
    {
        return reinterpret_cast<T>(m_Pointer + offset);
    }
    template <>
    inline Pointer Offset(std::uintptr_t offset) const
    {
        return m_Pointer + offset;
    }

    inline auto operator<=>(const Pointer& other) const = default;

  private:
    std::uintptr_t m_Pointer = 0;
};
