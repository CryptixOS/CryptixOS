/*
 * Created by v1tr10l7 on 91.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

namespace CommandLine
{
    void             Initialize();

    bool             Contains(std::string_view key);
    std::string_view GetString(std::string_view key);
}; // namespace CommandLine
