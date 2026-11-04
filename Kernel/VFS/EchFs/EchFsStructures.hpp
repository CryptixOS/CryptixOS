/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#pragma once

#include <Prism/Core/Types.hpp>
#define ECHFS_DEBUG 1
#if ECHFS_DEBUG == 1
    #define EchFsDebug(label, format, value)                                   \
                                                                               \
        Log::Message("{}{}{}: {" format "}\n", AnsiColor::FOREGROUND_MAGENTA,  \
                     label, AnsiColor::FOREGROUND_WHITE, value);
#else
    #define EchFsDebug(...)
#endif
