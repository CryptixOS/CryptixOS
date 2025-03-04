/*
 * Created by v1tr10l7 on 04.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Logger.hpp>
#include <Library/Module.hpp>

bool Probe()
{
    LogInfo("Hello, World from Kernel Module");

    return true;
}

MODULE_INIT(atkbd, Probe);
