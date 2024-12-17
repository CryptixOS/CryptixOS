/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

.intel_syntax noprefix
.code64

.section .text

.include "Arch/x86_64/common.inc"

.altmacro
.macro interrupt_handler int_number
interrupt_handler_\int_number:
.if \int_number != 8 && \int_number != 10 && \int_number != 11 && \int_number != 12 && \int_number != 13 && \int_number != 14 && \int_number != 17 && \int_number != 21 && \int_number != 29 && \int_number != 30
    push 0
.endif

    push \int_number
    cld

    test qword ptr [rsp + 24], 0x03
    je 1f
    swapgs
1:
    pushaq
    mov rdi, rsp

    call raiseInterrupt
    popaq
    add rsp, 0x10

    test qword ptr [rsp + 8], 0x03
    je 1f
    swapgs
1:
    iretq
.endmacro

.set i, 0
.rept 256
    interrupt_handler %i
    .set i, i+1
.endr

.section .data
.macro get_exception_addr num
    .quad interrupt_handler_\num
.endmacro

.global interrupt_handlers
interrupt_handlers:
.set i, 0
.rept 256
	get_exception_addr %i
    .set i, i+1
.endr
