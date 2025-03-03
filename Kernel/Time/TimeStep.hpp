/*
 * Created by v1tr10l7 on 18.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

class TimeStep final
{
  public:
    constexpr TimeStep() = default;
    constexpr TimeStep(u64 ns)
        : m_NanoSeconds(ns)
    {
    }

    constexpr       operator usize() const { return m_NanoSeconds; }

    constexpr usize Seconds() const { return Milliseconds() / 1000; }
    constexpr usize Milliseconds() const { return Microseconds() / 1000; }
    constexpr usize Microseconds() const { return Nanoseconds() / 1000; }
    constexpr usize Nanoseconds() const { return m_NanoSeconds; }

    constexpr auto  operator<=>(const TimeStep& other) const = default;

  private:
    usize m_NanoSeconds;
};

constexpr TimeStep operator*(usize count, TimeStep ts)
{
    return ts.Nanoseconds() * count;
}

constexpr usize operator""_ns(unsigned long long count) { return count; }
constexpr usize operator""_us(unsigned long long count)
{
    return count * 1000_ns;
}
constexpr usize operator""_ms(unsigned long long count)
{
    return count * 1000_us;
}
constexpr usize operator""_s(unsigned long long count)
{
    return count * 1000_ms;
}
