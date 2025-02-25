/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Limits.hpp>
#include <Prism/PathView.hpp>

#include <vector>

namespace Prism
{
    class Path
    {
      public:
        Path(const char path[])
            : m_Path(path)
        {
        }

        constexpr          operator PathView() const { return m_Path; }

        constexpr PathView GetView() const { return m_Path; }

        constexpr bool     IsEmpty() const { return m_Path.size() == 0; }
        constexpr bool     IsAbsolute() const
        {
            return !IsEmpty() && m_Path[0] == '/';
        }
        std::vector<std::string> SplitPath();

        constexpr int            Compare(const Path& str) const noexcept
        {
            return m_Path.compare(str.m_Path);
        }

        inline bool ValidateLength() const
        {
            return GetView().ValidateLength();
        }

      private:
        std::string m_Path;
    };

    constexpr bool operator==(const Path& lhs, const Path& rhs) noexcept
    {
        return lhs.Compare(rhs) == 0;
    }
    constexpr bool operator==(const Path& lhs, const char* rhs)
    {
        return lhs.Compare(rhs) == 0;
    }

    constexpr std::strong_ordering operator<=>(const Path& lhs,
                                               const Path& rhs) noexcept
    {
        return lhs.Compare(rhs) <=> 0;
    }

    constexpr std::strong_ordering operator<=>(const Path& lhs, const char* rhs)
    {
        return lhs.Compare(rhs) <=> 0;
    }
}; // namespace Prism
