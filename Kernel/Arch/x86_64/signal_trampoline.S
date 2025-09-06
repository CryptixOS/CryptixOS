/*
 * Created by v1tr10l7 on 04.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
.intel_syntax noprefix
.code64

.section .text
.global sigret_trampoline_start
sigret_trampoline_start:
// mov SYS_SIGRETURN number to rax
mov rax, 15
// call sigreturn
syscall
.global sigret_trampoline_end
sigret_trampoline_end:
