#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
ctos_kernel_sources(
    Arch.cpp
    Exceptions.cpp
    CPU.cpp
    Syscall.cpp
    VMM.cpp
    interrupts.S
    signal_trampoline.S
)

ctos_subdir(Drivers)

set(aarch64_args -mgeneral-regs-only -march=armv8-a+nofp+nosimd -mcmodel=small)
list(APPEND CTOS_KERNEL_C_ARGS ${aarch64_args})
list(APPEND CTOS_KERNEL_CXX_ARGS ${aarch64_args})
