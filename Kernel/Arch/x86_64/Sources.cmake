#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
ctos_kernel_sources(
    Arch.cpp
    CPU.cpp
    CMOS.cpp
    GDT.cpp
    IDT.cpp
    Syscall.cpp
    VMM.cpp
    idt.S
    memory.S
    signal_trampoline.S
    syscall.S
)

ctos_subdir(Drivers)

list(APPEND CTOS_KERNEL_C_ARGS
    -march=x86-64
    -msoft-float
    -mno-mmx
    -mno-sse
    -mno-sse2
    -mno-red-zone
    -mcmodel=kernel
)
list(APPEND CTOS_KERNEL_CXX_ARGS
    -march=x86-64
    -m64
    -msoft-float
    -mno-mmx
    -mno-sse
    -mno-sse2
    -mno-red-zone
    -mno-avx
    -mcmodel=kernel
    -mrdrnd
    -mrdseed
)
list(APPEND CTOS_KERNEL_LINK_ARGS -Wl,-z,noexecstack)
