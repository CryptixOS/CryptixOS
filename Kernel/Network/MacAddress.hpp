/*
 * Created by v1tr10l7 on 21.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Debug/Assertions.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Core/Types.hpp>

class [[gnu::packed]] MacAddress
{
  public:
    MacAddress() = default;
    explicit MacAddress(const Array<u8, 6> segments) { m_Segments = segments; }

    constexpr u8* Raw() { return m_Segments.Raw(); }

    constexpr u8  operator[](const usize index) const
    {
        Assert(index < m_Segments.Size());
        return m_Segments[index];
    }
    constexpr u8& operator[](const usize index)
    {
        Assert(index < m_Segments.Size());
        return m_Segments[index];
    }
    constexpr auto operator<=>(const MacAddress& other) const = default;

  private:
    Array<u8, 6> m_Segments;
};

template <>
struct fmt::formatter<MacAddress> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const MacAddress& addr, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(
            fmt::format("{:#x}:{:#x}:{:#x}:{:#x}:{:#x}:{:#x}", addr[0], addr[1],
                        addr[2], addr[3], addr[4], addr[5]),
            ctx);
    }
};
