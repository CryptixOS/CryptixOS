/*
 * Created by v1tr10l7 on 03.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
.section .text
.global memset2
.type memset2, %function
.align 6
memset2:
    # n == 0, so just return
    cbz x2, .done

    and x3, x1, 0xff
    mov x4, x3
    orr x4, x4, x4, LSL #8
    orr x4, x4, x4, LSL #16
    orr x4, x4, x4, LSL #32

    cmp x2, #8
    blt .byte_loop
.word_loop:
    str x4, [x0], #8
    subs x2, x2, #8
    bge .word_loop
.byte_loop:
    cbz x2, .done
    strb w3, [x0], #1
    subs x2, x2, #1
    bne .byte_loop
.done:
    ret
