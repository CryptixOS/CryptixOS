#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
ctos_kernel_sources(
    assert.cpp
    ctype.cpp
    errno.cpp
    stdio.cpp
    stdlib.cpp
    string.cpp
)

list(APPEND CTOS_KERNEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR})
