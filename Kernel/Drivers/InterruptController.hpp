/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Error.hpp>
#include <Prism/Memory/RefCounted.hpp>

class InterruptController : public RefCounted
{
  public:
    virtual ErrorOr<void> Initialize()     = 0;
    virtual ErrorOr<void> Shutdown()       = 0;

    virtual ErrorOr<void> Mask(u32 irq)    = 0;
    virtual ErrorOr<void> Unmask(u32 irq)  = 0;

    virtual ErrorOr<void> SendEOI(u32 irq) = 0;
};
