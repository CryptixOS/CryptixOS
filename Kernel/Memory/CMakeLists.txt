#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PRIVATE
    HigherHalfOffset.cpp
  PUBLIC FILE_SET CXX_MODULES FILES
    AddressRange.cppm
    AddressSpace.cppm
    IORegion.cppm
    MM.cppm
    MMIO.cppm
    MemoryZone.cppm
    PMM.cppm
    PageFault.cppm
    PageMap.cppm
    PageTableEntry.cppm
    Region.cppm
    ScopedMapping.cppm
    VMM.cppm
    AddressSpace.cpp
    MM.cpp
    PMM.cpp
    PageMap.cpp
    Region.cpp
    ScopedMapping.cpp
    VMM.cpp
)

add_subdirectory(Allocator)
