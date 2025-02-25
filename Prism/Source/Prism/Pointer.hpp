/*
 * Created by v1tr10l7 on 19.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>
#ifdef PRISM_TARGET_CRYPTIX
namespace VirtualMemoryManager
{
    extern usize GetHigherHalfOffset();
};
namespace VMM = VirtualMemoryManager;
#endif

namespace Prism
{
    struct Pointer
    {
        constexpr Pointer() = default;
        Pointer(std::nullptr_t) { m_Pointer = 0; }

        template <std::unsigned_integral T = std::uintptr_t>
            requires(std::is_integral_v<T>)
        constexpr Pointer(const T m_Pointer)
            : m_Pointer(m_Pointer)
        {
        }

        template <typename T = void*>
            requires(std::is_pointer_v<T>)
        Pointer(const T m_Pointer)
            : m_Pointer(reinterpret_cast<std::uintptr_t>(m_Pointer))
        {
        }

        constexpr operator std::uintptr_t() { return m_Pointer; }
        operator void*() { return reinterpret_cast<void*>(m_Pointer); }
        constexpr          operator bool() { return m_Pointer != 0; }
        constexpr Pointer& operator=(std::uintptr_t addr)
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
        constexpr T* As() const
        {
            return reinterpret_cast<T*>(m_Pointer);
        }
        template <typename T = std::uintptr_t>
        constexpr T Raw() const
        {
            return T(m_Pointer);
        }

        constexpr auto& operator->() { return *As<u64>(); }
        constexpr auto& operator*() { return *As<u64>(); }

#ifdef PRISM_TARGET_CRYPTIX
        bool IsHigherHalf() const
        {
            return m_Pointer >= VMM::GetHigherHalfOffset();
        }

        template <typename T = std::uintptr_t>
            requires(std::is_pointer_v<T> || std::is_integral_v<T>
                     || std::is_same_v<T, Pointer>)
        constexpr T ToHigherHalf() const
        {
            return IsHigherHalf() ? reinterpret_cast<T>(m_Pointer)
                                  : reinterpret_cast<T>(
                                      m_Pointer + VMM::GetHigherHalfOffset());
        }
        template <>
        constexpr Pointer ToHigherHalf() const
        {
            return ToHigherHalf<std::uintptr_t>();
        }

        template <typename T = std::uintptr_t>
            requires(std::is_pointer_v<T> || std::is_integral_v<T>
                     || std::is_same_v<T, Pointer>)
        constexpr T FromHigherHalf() const
        {
            return IsHigherHalf() ? reinterpret_cast<T>(
                       m_Pointer - VMM::GetHigherHalfOffset())
                                  : reinterpret_cast<T>(m_Pointer);
        }
        template <>
        constexpr Pointer FromHigherHalf() const
        {
            return FromHigherHalf<std::uintptr_t>();
        }
        constexpr Pointer FromHigherHalf() const
        {
            return IsHigherHalf() ? m_Pointer
                                  : m_Pointer + VMM::GetHigherHalfOffset();
        }

        template <typename T>
        constexpr T FromHigherHalf() const
        {
            return IsHigherHalf() ? reinterpret_cast<T>(
                       m_Pointer - VMM::GetHigherHalfOffset())
                                  : reinterpret_cast<T>(m_Pointer);
        }
#endif

        template <typename T = std::uintptr_t>
        constexpr T Offset(std::uintptr_t offset) const
        {
            return reinterpret_cast<T>(m_Pointer + offset);
        }
        template <>
        constexpr Pointer Offset(std::uintptr_t offset) const
        {
            return m_Pointer + offset;
        }

        Pointer& operator&(usize rhs)
        {
            m_Pointer &= rhs;
            return *this;
        }
        constexpr Pointer& operator|=(Pointer rhs)
        {
            m_Pointer |= rhs.m_Pointer;

            return *this;
        }
        constexpr auto operator<=>(const Pointer& other) const = default;

        constexpr      operator std::string() const
        {
            return std::to_string(m_Pointer);
        }

      private:
        std::uintptr_t m_Pointer = 0;
    };
}; // namespace Prism

template <>
struct fmt::formatter<Prism::Pointer> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const Prism::Pointer& addr, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(fmt::format("{}", 0), ctx);
    }
};
