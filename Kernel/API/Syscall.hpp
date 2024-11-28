/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

namespace Syscall
{
    struct Arguments
    {
        u64 index;
        u64 args[6];

        u64 returnValue;
    };

    void RegisterHandler(usize                                index,
                         std::function<uintptr_t(Arguments&)> handler,
                         std::string                          name);
#define RegisterSyscall(index, handler)                                        \
    ::Syscall::RegisterHandler(index, handler, #handler)

    void InstallAll();
    void Handle(Arguments& args);
}; // namespace Syscall
