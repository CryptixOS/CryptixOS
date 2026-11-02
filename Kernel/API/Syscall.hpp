/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#pragma once

#include <Common.hpp>
#include <Prism/Containers/Tuple.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/PathView.hpp>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>
#define RegisterSyscall(id, handler)                                           \
    s_Syscalls[id] = new Wrapper<decltype(handler)>(#handler, handler);
