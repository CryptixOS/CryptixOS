#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    Capability.cppm
    ClockID.cppm
    Credentials.cppm
    DeviceIDs.cppm
    MM.cppm
    Process.cppm
    Signals.cppm
    Sockets.cppm
    Syscall.cppm
    System.cppm
    Time.cppm
    VFS.cppm
    MM.cpp
    Process.cpp
    Syscall.cpp
    System.cpp
    Time.cpp
    VFS.cpp
)
