#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
ctos_kernel_sources(
    AddressSpace.cpp
    MM.cpp
    PMM.cpp
    PageMap.cpp
    Region.cpp
    ScopedMapping.cpp
    VMM.cpp
)

ctos_subdir(Allocator)
