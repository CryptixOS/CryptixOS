#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PRIVATE
    idt.S
    memory.S
    signal_trampoline.S
    syscall.S
  PUBLIC FILE_SET CXX_MODULES FILES
    Atomic.cppm
    CMOS.cppm
    CPU.cppm
    CPUID.cppm
    CPUIntrinsics.cppm
    ExecutionContext.cppm
    GDT.cppm
    IDT.cppm
    IO.cppm
    Types.cppm
    Arch.cpp
    CPU.cpp
    CMOS.cpp
    GDT.cpp
    IDT.cpp
    Syscall.cpp
    VMM.cpp
)

add_subdirectory(Drivers)
