/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Memory/VMM.hpp"

#include <vector>

using pid_t = i64;
using tid_t = i64;

enum class PrivilegeLevel
{
    ePrivileged,
    eUnprivileged,
};

struct Process
{
    Process() = default;
    Process(std::string_view name)
        : name(name)
        , pageMap(nullptr)
        , nextTid(1)
        , parent(nullptr)
        , userStackTop(0x70000000000)

    {
    }

    pid_t                               pid;
    std::string                         name;
    PageMap*                            pageMap;
    std::atomic<tid_t>                  nextTid;

    Process*                            parent;
    std::vector<struct Thread*>         threads;

    std::vector<struct FileDescriptor*> fileDescriptors;
    uintptr_t                           userStackTop;
};
