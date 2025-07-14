/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>

namespace Serial
{
    bool               Initialize();

    u8                 Read();
    void               Write(u8 data);

    constexpr static isize Write(StringView str)
    {
        isize nwritten = 0;
        for (auto c : str) Write(c), ++nwritten;

        return nwritten;
    }
    constexpr isize Write(u8* data, usize size)
    {
        return Write(StringView(reinterpret_cast<char*>(data), size));
    }
}; // namespace Serial
