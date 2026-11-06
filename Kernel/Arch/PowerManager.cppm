/*
 * Created by v1tr10l7 on 06.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
// `import Kernel.Arch.PowerManager;`
//
// The declarations are exported as extern "C++", which attaches them to the
// global module: the symbols keep their usual mangled names (ksyms.sym and the
// .ko modules resolve them by name), and Arch/PowerManager.cpp, which is not a
// module unit, still defines them.
export module Kernel.Arch.PowerManager;

export extern "C++"
{
    namespace PowerManager
    {
        void Reboot();
        void PowerOff();

        void Halt();
        void Suspend();
    }; // namespace PowerManager
}
