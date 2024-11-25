/*
 * Created by v1tr10l7 on 24.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
.text
handle_sync:
    // Push all regular registers */
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    /* combine x30 (link register) with stack */
    mrs x0, SP_EL0
    stp x30, x0, [sp, #-16]!
    /* Pass the current stack as the first arg (struct regs *r) */
    mov x0, sp
    /* Make sure x18 has our kernel CPU-local pointer */
    mrs x18, TPIDR_EL1
    .extern raiseSyncException
    bl raiseSyncException

    mrs x0, SPSR_EL1
    and w0, w0, #0x200000
    cbz w0, _sync_cont
    mrs x0, MDSCR_EL1
    orr x0, x0, #1
    msr MDSCR_EL1, x0
    _sync_cont:

    ldp x30, x0, [sp], #16
    msr SP_EL0, x0
    ldp x28, x29, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    eret

handle_fault:
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    mrs x0, SP_EL0
    stp x30, x0, [sp, #-16]!
    mov x0, sp
    mrs x18, TPIDR_EL1
    .extern raiseFault
    bl raiseFault
    ldp x30, x0, [sp], #16
    msr SP_EL0, x0
    ldp x28, x29, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    eret

handle_irq:
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    mrs x0, SP_EL0
    stp x30, x0, [sp, #-16]!
    mov x0, sp
    mrs x18, TPIDR_EL1
    .extern raiseIrq
    bl raiseIrq
    mrs x0, SPSR_EL1
    and w0, w0, #0x200000
    cbz w0, _irq_cont
    mrs x0, MDSCR_EL1
    orr x0, x0, #1
    msr MDSCR_EL1, x0
    _irq_cont:
    ldp x30, x0, [sp], #16
    msr SP_EL0, x0
    ldp x28, x29, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    eret

.global exception_handlers
.balign 0x800
exception_handlers:
exception_sp0_sync:
    b .
.balign 0x80
exception_sp0_irq:
    b .
.balign 0x80
exception_sp0_fiq:
    b .
.balign 0x80
exception_sp0_serror:
    b .

// EL1 exception - these could happen only in the kernel
.balign 0x80
exception_spx_sync:
    b handle_fault
.balign 0x80
exception_spx_irq:
    b .
.balign 0x80
exception_spx_fiq:
    b .
.balign 0x80
exception_spx_serror:
    b .

// EL0-El1 transition handlers
.balign 0x80
exception_lower_sync:
    b handle_sync

// Interrupts
.balign 0x80
exception_lower_irq:
    b handle_irq

// Fast Interrupts
.balign 0x80
exception_lower_fiq:
    b .

.balign 0x80
exception_lower_serror:
    b .
.balign 0x80

// 32-bit userspace
exception_lower_32_sync:
    b .
.balign 0x80
exception_lower_32_irq:
    b .
.balign 0x80
exception_lower_32_fiq:
    b .
.balign 0x80
exception_lower_32_serror:
    b .


