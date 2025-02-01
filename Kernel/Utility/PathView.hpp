/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

#include <string>

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

    bool                  ValidateLength();
    constexpr bool        IsEmpty() const { return m_Path.empty(); }

    constexpr             operator std::string_view() const { return m_Path; }

    constexpr const char& operator[](usize i) const { return m_Path[i]; }

    constexpr usize       GetSize() const { return m_Path.size(); }
    constexpr const char* Raw() const { return m_Path.data(); }

    std::string_view      GetLastComponent() const;

  private:
    std::string_view m_Path;
};
