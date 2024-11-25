/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "IDT.hpp"

#include "Common.hpp"

#include "Arch/x86_64/CPUContext.hpp"
#include "Arch/x86_64/GDT.hpp"

extern const char*             exceptionNames[];

constexpr u32                  MAX_IDT_ENTRIES     = 256;
// constexpr u32      IDT_ENTRY_PRESENT   = BIT(7);

[[maybe_unused]] constexpr u32 GATE_TYPE_INTERRUPT = 0xe;
[[maybe_unused]] constexpr u32 GATE_TYPE_TRAP      = 0xf;

struct IDTEntry
{
    u16 isrLow;
    u16 kernelCS;
    u8  ist;
    union
    {
        u8 attributes;
        struct
        {
            u8 gateType : 4;
            u8 unused   : 1;
            u8 dpl      : 2;
            u8 present  : 1;
        } __attribute__((packed));
    };
    u16 isrMiddle;
    u32 isrHigh;
    u32 reserved;
} __attribute__((packed));

[[maybe_unused]] alignas(0x10) static IDTEntry idtEntries[256] = {};
extern "C" void* interrupt_handlers[];

static void      idtWriteEntry(u16 vector, uintptr_t handler, u8 attributes)
{
    Assert(vector < MAX_IDT_ENTRIES);
    IDTEntry* entry   = idtEntries + vector;

    entry->isrLow     = handler & 0xffff;
    entry->kernelCS   = GDT::KERNEL_CODE_SELECTOR;
    entry->attributes = attributes;
    entry->reserved   = 0;
    entry->ist        = 0;
    entry->isrMiddle  = (handler & 0xffff0000) >> 16;
    entry->isrHigh    = (handler & 0xffffffff00000000) >> 32;
}

[[noreturn]] [[maybe_unused]]
static void raiseException(CPUContext* ctx)
{
    // TODO(v1tr10l7): Display valid cpu ids when smp will be implemented
    u64 cpuID = 0;
    EarlyPanic(
        "Captured exception[%#x] on cpu %zu: '%s'\n\rError Code: "
        "%#b\n\rrip: "
        "%#p",
        ctx->interruptVector, cpuID, exceptionNames[ctx->interruptVector],
        ctx->errorCode, ctx->rip);

    Arch::Halt();
}

[[noreturn]]
static void unhandledInterrupt(CPUContext* context)
{
    LogError("\nAn unhandled interrupt 0x{:x} occurred",
             context->interruptVector);

    for (;;) __asm__ volatile("cli; hlt");
}
extern "C" void raiseInterrupt(CPUContext* ctx)
{
    if (ctx->interruptVector < 0x20) raiseException(ctx);
    unhandledInterrupt(ctx);
}

namespace IDT
{
    void Initialize()
    {
        for (u32 i = 0; i < 256; i++)
        {
            idtWriteEntry(i, reinterpret_cast<uintptr_t>(interrupt_handlers[i]),
                          0x80 | GATE_TYPE_INTERRUPT);
        }

        LogInfo("IDT: Initialized!");
    }
    void Load()
    {
        struct
        {
            u16       limit;
            uintptr_t base;
        } __attribute__((packed)) idtr;
        idtr.limit = sizeof(idtEntries) - 1;
        idtr.base  = reinterpret_cast<uintptr_t>(idtEntries);
        __asm__ volatile("lidt %0" : : "m"(idtr));
    }

} // namespace IDT

#pragma region exception_names
const char*    exceptionNames[] = {
    "Divide-by-zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device not available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved",
    "Triple Fault",
    "FPU Error Interrupt",
};
#pragma endregion
