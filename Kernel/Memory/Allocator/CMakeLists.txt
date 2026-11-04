#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PRIVATE
    Operators.cpp
  PUBLIC FILE_SET CXX_MODULES FILES
    BitmapAllocator.cppm
    BuddyAllocator.cppm
    KernelHeap.cppm
    PageFrameAllocator.cppm
    BitmapAllocator.cpp
    BuddyAllocator.cpp
    KernelHeap.cpp
)
