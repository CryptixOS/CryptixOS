/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#pragma once

#include <Prism/Memory/Buffer.hpp>
#define CTOS_DEBUG_TMPFS 1
#if CTOS_DEBUG_TMPFS != 0
    #define TmpFsTrace(...)                                                    \
        if (g_LogTmpFs) LogMessage("[\e[35mTmpFs\e[0m]: {}\n", __VA_ARGS__)
#else
    #define TmpFsTrace(...)
#endif
