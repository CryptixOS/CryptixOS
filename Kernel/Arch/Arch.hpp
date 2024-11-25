/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Types.hpp"

#include <string_view>

namespace Arch
{
    void                           Initialize();

    __attribute__((noreturn)) void Halt();
    void                           Pause();

    void                           EnableInterrupts();
    void                           DisableInterrupts();
    bool                           ExchangeInterruptFlag(bool enabled);
}; // namespace Arch
