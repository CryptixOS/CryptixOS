/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Arch/InterruptManager.hpp"

#include "Arch/x86_64/GDT.hpp"
#include "Arch/x86_64/IDT.hpp"

namespace InterruptManager
{
    void InstallExceptionHandlers()
    {
        GDT::Initialize();
        GDT::Load(0);

        IDT::Initialize();
        IDT::Load();
    }
}; // namespace InterruptManager
