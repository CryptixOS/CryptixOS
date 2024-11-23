/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Types.hpp"

namespace Stacktrace
{
    bool Initialize();
    void Print(usize maxframes = 16);
}; // namespace Stacktrace
