/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <System/InterruptHandler.hpp>

class Device;
namespace InterruptManager
{
    void                     InstallExceptions();

    Ref<InterruptController> Controller();
    void                     SetController(Ref<InterruptController> ctrl);

    Ref<InterruptDispatcher> AllocateHandler(u32 irq, Device* device,
                                             StringView moduleName);

    void                     Mask(u32 irq);
    void                     Unmask(u32 irq);

    void                     SendEOI(u32 irq);
}; // namespace InterruptManager
