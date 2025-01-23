/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Limits.hpp>
#include <Utility/PathView.hpp>

#include <string_view>
#include <vector>

class Path
{
  public:
    Path(const char* path)
        : m_Path(path)
    {
    }

    static bool                     IsAbsolute(std::string_view path);
    static bool                     ValidateLength(PathView path);

    static std::vector<std::string> SplitPath(std::string path);

  private:
    std::string m_Path;
};
