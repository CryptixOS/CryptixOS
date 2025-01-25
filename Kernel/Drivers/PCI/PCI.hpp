/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Spinlock.hpp>
#include <Utility/Types.hpp>

namespace PCI
{
    void                  Initialize();
    class HostController* GetHostController(u32 domain);

    constexpr usize       PCI_CONFIG_ADDRESS = 0x0cf8;
    constexpr usize       PCI_CONFIG_DATA    = 0x0cfc;

    constexpr u16         PCI_INVALID        = 0xffff;
}; // namespace PCI
