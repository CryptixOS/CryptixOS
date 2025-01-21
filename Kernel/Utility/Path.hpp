/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Limits.hpp>

#include <string_view>
#include <vector>

using PathView = std::string_view;

namespace Path
{
    bool                     IsAbsolute(std::string_view path);
    bool                     ValidateLength(PathView path);

    std::vector<std::string> SplitPath(std::string path);
} // namespace Path
