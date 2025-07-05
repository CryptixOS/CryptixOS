/*
 * Created by v1tr10l7 on 91.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Optional.hpp>

namespace CommandLine
{
    void                Initialize();

    bool                Contains(StringView key);
    StringView          GetString(StringView key);
    Optional<bool> GetBoolean(StringView key);
}; // namespace CommandLine
