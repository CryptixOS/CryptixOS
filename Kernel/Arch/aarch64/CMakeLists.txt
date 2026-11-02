#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PRIVATE
    Syscall.cpp
    interrupts.S
    signal_trampoline.S
  PUBLIC FILE_SET CXX_MODULES FILES
    CPU.cppm
    CPUIntrinsics.cppm
    ExecutionContext.cppm
    Arch.cpp
    CPU.cpp
    InterruptManager.cpp
    VMM.cpp
)

add_subdirectory(Drivers)
