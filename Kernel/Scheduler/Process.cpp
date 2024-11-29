/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Process.hpp"

inline usize AllocatePid()
{
    static std::atomic<pid_t> pid = 7;

    return pid++;
}

Process::Process(std::string_view name, PrivilegeLevel ring)
    : name(name)
    , ring(ring)
{
    pid = AllocatePid();
    if (ring == PrivilegeLevel::ePrivileged) pageMap = VMM::GetKernelPageMap();
}
