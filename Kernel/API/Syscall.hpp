/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include <utility>

namespace Syscall
{
    struct Arguments
    {
        u64 index;
        u64 args[6];

        u64 returnValue;
    };

    enum class ID
    {
        eRead   = 0,
        eWrite  = 1,
        eOpen   = 2,
        eClose  = 3,
        eStat   = 4,
        eFStat  = 5,
        eLStat  = 6,
        ePoll   = 7,
        eLSeek  = 8,
        eIoCtl  = 16,
        eExit   = 60,
        eGetTid = 186,
    };

    void RegisterHandler(usize                                index,
                         std::function<uintptr_t(Arguments&)> handler,
                         std::string                          name);
#define RegisterSyscall(index, handler)                                        \
    ::Syscall::RegisterHandler(std::to_underlying(index), handler, #handler)

    void InstallAll();
    void Handle(Arguments& args);
}; // namespace Syscall
