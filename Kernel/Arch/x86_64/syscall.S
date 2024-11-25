/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
.intel_syntax noprefix
.code64

.section .text
.extern handleSyscall


.include "Arch/x86_64/common.inc"

syscall_entry:
    swapgs

    mov gs:[16], rsp
    mov rsp, gs:[24]
 
    // ss
    push 0x1b
    // rsp
    push gs:[16] 
    // rflags
    push r11 
    // cs
    push 0x23 
    // rip
    push rcx 
    // error_code
    push 0 
    // int_no
    push 0 

    pushaq

    sti
    mov rdi, rsp
    call handleSyscall
    cli

    popaq

    add rsp, 56
    mov rsp, gs:[16]

    swapgs
    sysretq
.global syscall_entry

