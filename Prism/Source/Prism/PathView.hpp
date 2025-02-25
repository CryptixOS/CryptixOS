/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>

#include <vector>

namespace Prism
{
    class PathView
    {
      public:
        constexpr PathView(std::string path)
            : m_Path(path)
        {
        }
        constexpr PathView(const char* path)
            : m_Path(path)
        {
        }

        constexpr PathView(std::string_view path)
            : m_Path(path)
        {
        }

        bool           ValidateLength();
        constexpr bool IsEmpty() const { return m_Path.empty(); }

        inline bool    IsAbsolute() const
        {
            return !IsEmpty() && m_Path[0] == '/';
        }
        std::vector<std::string> SplitPath();

        constexpr operator std::string_view() const { return m_Path; }

        constexpr const char& operator[](usize i) const { return m_Path[i]; }

        constexpr int         Compare(const PathView& str) const noexcept
        {
            return m_Path.compare(str.m_Path);
        }

        constexpr usize       GetSize() const { return m_Path.size(); }
        constexpr const char* Raw() const { return m_Path.data(); }

        std::string_view      GetLastComponent() const;

      private:
        std::string_view m_Path;
    };

    constexpr bool operator==(const PathView& lhs, const PathView& rhs) noexcept
    {
        return lhs.Compare(rhs) == 0;
    }

    constexpr std::strong_ordering operator<=>(const PathView& lhs,
                                               const PathView& rhs) noexcept
    {
        return lhs.Compare(rhs) <=> 0;
    }
}; // namespace Prism
