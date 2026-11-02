#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    BootInfo.cppm
    BootMemoryInfo.cppm
    BootModuleInfo.cppm
    CommandLine.cppm
    BootInfo.cpp
    CommandLine.cpp
)
