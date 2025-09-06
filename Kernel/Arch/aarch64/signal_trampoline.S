/*
 * Created by v1tr10l7 on 04.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
.section .text
.global sigret_trampoline_start
sigret_trampoline_start:
    // mov SYS_SIGRETURN number to x0 (on ARM, syscall numbers are passed in x0)
    mov x0, #15
    // svc 0 is used to make a system call in ARM (equivalent to syscall in x86)
    svc #0
sigret_trampoline_end:

