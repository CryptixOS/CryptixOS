#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    Fat32Fs.cppm
    Fat32FsINode.cppm
    Fat32FsStructures.cppm
    Fat32Fs.cpp
    Fat32FsINode.cpp
)
