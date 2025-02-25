/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

namespace API::MM
{
    ErrorOr<intptr_t> SysMMap(PM::Pointer addr, usize len, i32 prot, i32 flags,
                              i32 fdNum, off_t offset);
}

namespace Syscall::MM
{
    ErrorOr<intptr_t> SysMMap(Syscall::Arguments& args);
    ErrorOr<i32>      SysMUnMap(Arguments& args);
} // namespace Syscall::MM
