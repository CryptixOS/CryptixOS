/*
 * Created by v1tr10l7 on 24.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
.text

#define REGISTER_STATE_SIZE (36 * 8)
#if REGISTER_STATE_SIZE % 16 != 0
#    error "REGISTER_STATE_SIZE must be multiple of 16 bytes!"
#endif

#define SPSR_EL1_SLOT       (31 * 8)
#define ELR_EL1_SLOT        (32 * 8)
#define SP_EL0_SLOT         (33 * 8)
#define TPIDR_EL0_SLOT      (34 * 8)
#define ESR_EL1_SLOT        (35 * 8)
#define TRAP_FRAME_SIZE     REGISTER_STATE_SIZE
.macro save_current_context
    // Allocate stack space for full ExecutionContext
    sub sp, sp, #TRAP_FRAME_SIZE

    // Save general-purpose registers x0–x30
    stp x0, x1,     [sp, #(0*8)]
    stp x2, x3,     [sp, #(2*8)]
    stp x4, x5,     [sp, #(4*8)]
    stp x6, x7,     [sp, #(6*8)]
    stp x8, x9,     [sp, #(8*8)]
    stp x10, x11,   [sp, #(10*8)]
    stp x12, x13,   [sp, #(12*8)]
    stp x14, x15,   [sp, #(14*8)]
    stp x16, x17,   [sp, #(16*8)]
    stp x18, x19,   [sp, #(18*8)]
    stp x20, x21,   [sp, #(20*8)]
    stp x22, x23,   [sp, #(22*8)]
    stp x24, x25,   [sp, #(24*8)]
    stp x26, x27,   [sp, #(26*8)]
    stp x28, x29,   [sp, #(28*8)]
    str x30,        [sp, #(30*8)]

    // Save special system registers
    mrs x0, spsr_el1
    str x0, [sp, #SPSR_EL1_SLOT]
    mrs x0, elr_el1
    str x0, [sp, #ELR_EL1_SLOT]
    mrs x0, sp_el0
    str x0, [sp, #SP_EL0_SLOT]
    mrs x0, tpidr_el0
    str x0, [sp, #TPIDR_EL0_SLOT]
    mrs x0, esr_el1
    str x0, [sp, #ESR_EL1_SLOT]

    // Pass pointer to ExecutionContext in x0
    mov x0, sp
.endmacro

.macro restore_context
    // x0 contains pointer to ExecutionContext
    mov sp, x0

    // Restore special registers first
    ldr x1, [sp, #SPSR_EL1_SLOT]
    msr spsr_el1, x1
    ldr x1, [sp, #ELR_EL1_SLOT]
    msr elr_el1, x1
    ldr x1, [sp, #SP_EL0_SLOT]
    msr sp_el0, x1
    ldr x1, [sp, #TPIDR_EL0_SLOT]
    msr tpidr_el0, x1
    // ESR_EL1 is read-only; do not restore

    // Restore general-purpose registers
    ldp x0, x1, [sp, #(0*8)]
    ldp x2, x3, [sp, #(2*8)]
    ldp x4, x5, [sp, #(4*8)]
    ldp x6, x7, [sp, #(6*8)]
    ldp x8, x9, [sp, #(8*8)]
    ldp x10, x11, [sp, #(10*8)]
    ldp x12, x13, [sp, #(12*8)]
    ldp x14, x15, [sp, #(14*8)]
    ldp x16, x17, [sp, #(16*8)]
    ldp x18, x19, [sp, #(18*8)]
    ldp x20, x21, [sp, #(20*8)]
    ldp x22, x23, [sp, #(22*8)]
    ldp x24, x25, [sp, #(24*8)]
    ldp x26, x27, [sp, #(26*8)]
    ldp x28, x29, [sp, #(28*8)]
    ldr x30, [sp, #(30*8)]

    // Free stack space
    add sp, sp, #TRAP_FRAME_SIZE

    // Return from exception
    eret
.endmacro

.global raiseSyncException
.global handle_sync
handle_sync:
    save_current_context
    mrs x18, TPIDR_EL1      // kernel-local CPU pointer if needed
    bl raiseSyncException        // x0 points to ExecutionContext
    restore_context

.global raiseIrq
.global handle_irq
handle_irq:
    save_current_context
    mrs x18, TPIDR_EL1
    bl raiseIrq
    restore_context

.global raiseFault
.global handle_fault
handle_fault:
    save_current_context
    mrs x18, TPIDR_EL1
    bl raiseFault
    restore_context

// FIQ and SError handlers can be stubs for now
.global handle_fiq
handle_fiq:
    b .

.global handle_serror
handle_serror:
    b .

// Align vectors according to AArch64 spec
.global exception_vectors
.balign 0x800       // 1KB alignment recommended for vector table
exception_vectors:
    // ---- Current EL with SP0 (EL1 kernel stack) ----
    // Synchronous
    b handle_sync
    // IRQ
    b handle_irq
    // FIQ
    b .
    // SError
    b .

    // ---- Current EL with SPx (EL1, other stack) ----
    b handle_sync   // synchronous
    b handle_irq
    b .
    b .

    // ---- Lower EL using AArch64 (EL0 → EL1) ----
    // Synchronous (e.g., system call, page fault)
    b handle_sync
    // IRQ
    b handle_irq
    // FIQ
    b .
    // SError
    b .

    // ---- Lower EL using AArch32 (32-bit) ----
    b .
    b .
    b .
    b .
