/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/InterruptHandler.hpp>

#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/CPUContext.hpp>
#include <Arch/x86_64/Drivers/PIC.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Firmware/ACPI/MADT.hpp>

#include <Memory/PageFault.hpp>
#include <Prism/Containers/Array.hpp>

#pragma region exception_names
auto s_ExceptionNames = ToArray({
    "Divide-by-zero"_sv,
    "Debug"_sv,
    "Non-Maskable Interrupt"_sv,
    "Breakpoint"_sv,
    "Overflow"_sv,
    "Bound Range Exceeded"_sv,
    "Invalid Opcode"_sv,
    "Device not available"_sv,
    "Double Fault"_sv,
    "Coprocessor Segment Overrun"_sv,
    "Invalid TSS"_sv,
    "Segment Not Present"_sv,
    "Stack-Segment Fault"_sv,
    "General Protection Fault"_sv,
    "Page Fault"_sv,
    "Reserved"_sv,
    "x87 Floating-Point Exception"_sv,
    "Alignment Check"_sv,
    "Machine Check"_sv,
    "SIMD Floating-Point Exception"_sv,
    "Control Protection Exception"_sv,
    "Reserved"_sv,
    "Reserved"_sv,
    "Reserved"_sv,
    ""_sv,
    "Virtualization Exception"_sv,
    "Reserved"_sv,
    "Reserved"_sv,
    "Hypervisor Injection Exception"_sv,
    "VMM Communication Exception"_sv,
    "Security Exception"_sv,
    "Reserved"_sv,
    "Triple Fault"_sv,
    "FPU Error Interrupt"_sv,
});
#pragma endregion

constexpr u32 MAX_IDT_ENTRIES     = 256;
constexpr u32 IDT_ENTRY_PRESENT   = Bit(7);

constexpr u32 GATE_TYPE_INTERRUPT = 0xe;
constexpr u32 GATE_TYPE_TRAP      = 0xf;

namespace Exception
{
    constexpr u8 BREAKPOINT = 0x03;
    constexpr u8 PAGE_FAULT = 0x0e;
}; // namespace Exception
namespace MemoryManager
{
    void HandlePageFault(const PageFaultInfo& info);
};

struct [[gnu::packed]] IDTEntry
{
    u16 IsrLow;
    u16 KernelCS;
    u8  IST;
    union
    {
        u8 Attributes;
        struct [[gnu::packed]]
        {
            u8 GateType : 4;
            u8 Unused   : 1;
            u8 DPL      : 2;
            u8 Present  : 1;
        };
    };
    u16 IsrMiddle;
    u32 IsrHigh;
    u32 Reserved;
};

[[maybe_unused]] alignas(0x10) static IDTEntry s_IdtEntries[256] = {};
extern "C" void*        interrupt_handlers[];
static InterruptHandler s_InterruptHandlers[256];
static void (*exceptionHandlers[32])(CPUContext*);

static void idtWriteEntry(u16 vector, uintptr_t handler, u8 attributes)
{
    Assert(vector < MAX_IDT_ENTRIES);
    IDTEntry* entry   = s_IdtEntries + vector;

    entry->IsrLow     = handler & 0xffff;
    entry->KernelCS   = GDT::KERNEL_CODE_SELECTOR;
    entry->Attributes = attributes;
    entry->Reserved   = 0;
    entry->IST        = 0;
    entry->IsrMiddle  = (handler & 0xffff0000) >> 16;
    entry->IsrHigh    = (handler & 0xffffffff00000000) >> 32;
}

[[noreturn]] static void raiseException(CPUContext* ctx)
{
    u64 cpuID = CPU::GetCurrentID();

    EarlyPanic(
        "Captured exception[%#x] on cpu %zu: '%s'\n\rError Code: "
        "%#b\n\rrip: "
        "%#p",
        ctx->interruptVector, cpuID, s_ExceptionNames[ctx->interruptVector],
        ctx->errorCode, ctx->rip);

    Arch::Halt();
}

// TODO(v1tr10l7): properly handle breakpoints
static void            breakpoint(CPUContext* ctx) { EarlyPanic("Breakpoint"); }

constexpr usize        PAGE_FAULT_PRESENT           = Bit(0);
constexpr usize        PAGE_FAULT_WRITE             = Bit(1);
constexpr usize        PAGE_FAULT_USER              = Bit(2);
constexpr usize        PAGE_FAULT_RESERVED_WRITE    = Bit(3);
constexpr usize        PAGE_FAULT_INSTRUCTION_FETCH = Bit(4);
constexpr usize        PAGE_FAULT_PROTECTION_KEY    = Bit(5);
constexpr usize        PAGE_FAULT_SHADOW_STACK      = Bit(6);
constexpr usize        PAGE_FAULT_SGX               = Bit(7);

inline PageFaultReason pageFaultReason(u64 errorCode)
{
    PageFaultReason reason;
    if (!(errorCode & PAGE_FAULT_PRESENT))
        reason |= PageFaultReason::eNotPresent;
    if (errorCode & PAGE_FAULT_WRITE) reason |= PageFaultReason::eWrite;
    if (errorCode & PAGE_FAULT_USER) reason |= PageFaultReason::eUser;
    if (errorCode & PAGE_FAULT_RESERVED_WRITE)
        reason |= PageFaultReason::eReservedWrite;
    if (errorCode & PAGE_FAULT_INSTRUCTION_FETCH)
        reason |= PageFaultReason::eInstructionFetch;
    if (errorCode & PAGE_FAULT_PROTECTION_KEY)
        reason |= PageFaultReason::eProtectionKey;
    if (errorCode & PAGE_FAULT_SHADOW_STACK)
        reason |= PageFaultReason::eShadowStack;
    if (errorCode & PAGE_FAULT_SGX)
        reason |= PageFaultReason::eSoftwareGuardExtension;

    return reason;
}

static void pageFault(CPUContext* ctx)
{
    Pointer       faultAddress = CPU::ReadCR2();
    auto          errorCode    = ctx->errorCode;
    auto          faultReason  = pageFaultReason(errorCode);

    PageFaultInfo faultInfo(faultAddress, faultReason, ctx);
    MemoryManager::HandlePageFault(faultInfo);
}

[[noreturn]] static void unhandledInterrupt(CPUContext* context)
{
    EarlyLogError("\nAn unhandled interrupt %#x occurred",
                  context->interruptVector);

    for (;;) __asm__ volatile("cli; hlt");
}
extern "C" void raiseInterrupt(CPUContext* ctx)
{
    auto& handler = s_InterruptHandlers[ctx->interruptVector];

    if (ctx->interruptVector < 0x20)
        return exceptionHandlers[ctx->interruptVector](ctx);
    else if (handler.IsUsed())
    {
        if (handler.eoiFirst) InterruptManager::SendEOI(ctx->interruptVector);
        handler(ctx);
        if (!handler.eoiFirst) InterruptManager::SendEOI(ctx->interruptVector);

        return;
    }

    unhandledInterrupt(ctx);
}

namespace IDT
{
    void Initialize()
    {
        for (u32 i = 0; i < 256; i++)
        {

            idtWriteEntry(
                i, reinterpret_cast<uintptr_t>(interrupt_handlers[i]),
                IDT_ENTRY_PRESENT
                    | (i < 0x20 ? GATE_TYPE_TRAP : GATE_TYPE_INTERRUPT));

            s_InterruptHandlers[i].SetInterruptVector(i);
            if (i < 32) exceptionHandlers[i] = raiseException;
        }

        exceptionHandlers[Exception::BREAKPOINT] = breakpoint;
        exceptionHandlers[Exception::PAGE_FAULT] = pageFault;

        SetDPL(Exception::BREAKPOINT, 3);
        LogInfo("IDT: Initialized!");
    }
    void Load()
    {
        struct [[gnu::packed]]
        {
            u16      Limit;
            upointer Base;
        } idtr;
        idtr.Limit = sizeof(s_IdtEntries) - 1;
        idtr.Base  = reinterpret_cast<upointer>(s_IdtEntries);
        __asm__ volatile("lidt %0" : : "m"(idtr));
    }

    InterruptHandler* AllocateHandler(u8 hint)
    {
        LogTrace("IDT: Allocating handler...");
        if (hint < 0x20) hint += 0x20;

        if (MADT::LegacyPIC())
        {
            if ((hint >= 0x20 && hint <= (0x20 + 15))
                && !s_InterruptHandlers[hint].IsUsed())
                return &s_InterruptHandlers[hint];
        }

        for (usize i = hint; i < 256; i++)
        {
            if (!s_InterruptHandlers[i].IsUsed()
                && !s_InterruptHandlers[i].IsReserved())
            {
                s_InterruptHandlers[i].Reserve();
                s_InterruptHandlers[i].SetInterruptVector(i);
                return &s_InterruptHandlers[i];
            }
        }

        Panic("IDT: Out of interrupt handlers");
    }
    InterruptHandler* GetHandler(u8 vector)
    {
        return &s_InterruptHandlers[vector];
    }

    void SetIST(u8 vector, u32 value) { s_IdtEntries[vector].IST = value; }
    void SetDPL(u8 vector, u8 dpl) { s_IdtEntries[vector].DPL = dpl; }
} // namespace IDT
