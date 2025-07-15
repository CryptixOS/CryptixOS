/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/InterruptHandler.hpp>
#include <Prism/Core/Error.hpp>

class InterruptController 
{
    public:
        virtual ErrorOr<void> Initialize() = 0;
        virtual ErrorOr<void> Shutdown() = 0;

        virtual ErrorOr<InterruptHandler*> AllocateHandler(u8 hint = 0x20 + 0x10) = 0;
        
        virtual ErrorOr<void> Mask(u8 irq) = 0;
        virtual ErrorOr<void> Unmask(u8 irq) = 0;

        virtual ErrorOr<void> SendEOI(u8 vector);
};
