#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
list(APPEND CTOS_MODULE_ARGS
    -msoft-float
    -mno-mmx
    -mno-sse
    -mno-sse2
    -mno-red-zone
    -mcmodel=large
)

ctos_subdir(input)
ctos_subdir(pcspk)
