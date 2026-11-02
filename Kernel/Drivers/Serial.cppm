/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>

export module Cryptix:Drivers.Serial;

namespace Serial
{
    export bool               Initialize();

    export u8                 Read();
    export void               Write(u8 data);

    export inline isize Write(StringView str)
    {
        isize nwritten = 0;
        for (auto c : str) Write(c), ++nwritten;

        return nwritten;
    }
    export inline isize Write(u8* data, usize size)
    {
        return Write(StringView(reinterpret_cast<char*>(data), size));
    }
}; // namespace Serial
