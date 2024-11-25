/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

class InterruptHandler;
namespace InterruptManager
{
    void              InstallExceptionHandlers();

    InterruptHandler* AllocateHandler(u8 hint = 0x20);

    void              Mask(u8 vector);
    void              Unmask(u8 vector);
}; // namespace InterruptManager
