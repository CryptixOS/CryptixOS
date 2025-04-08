/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Prism/Core/Types.hpp"

#include <string_view>

namespace Serial
{
    bool               Initialize();

    u8                 Read();
    void               Write(u8 data);

    inline static void Write(std::string_view str)
    {
        for (auto c : str) Write(c);
    }
    inline void Write(u8* data, usize size)
    {
        while (size-- > 0) Write(*data);
    }
}; // namespace Serial
