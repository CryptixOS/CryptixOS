/*
 * Created by v1tr10l7 on 08.02.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/TTY/MasterPTY.hpp>

MasterPTY::MasterPTY(StringView name, usize minor)
    : TTY(name, minor)
{
}
