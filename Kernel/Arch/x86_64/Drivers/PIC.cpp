/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Arch/x86_64/Drivers/PIC.hpp>
#include <Arch/x86_64/IO.hpp>
#include <Arch/x86_64/IDT.hpp>

// https://pdos.csail.mit.edu/6.828/2005/readings/hardware/8259A.pdf
inline static constexpr const u32 PIC1          = 0x20;
inline static constexpr const u32 PIC1_COMMAND  = PIC1 + 0;
inline static constexpr const u32 PIC1_DATA     = PIC1 + 1;

inline static constexpr const u32 PIC2          = 0xa0;
inline static constexpr const u32 PIC2_COMMAND  = PIC2 + 0;
inline static constexpr const u32 PIC2_DATA     = PIC2 + 1;

inline static constexpr const u32 OCW2_SEND_EOI = 0x20;

inline static constexpr const u32 OCW3_GET_IRR  = 0x0a;
inline static constexpr const u32 OCW3_GET_ISR  = 0x0b;

enum class ICW1Flags : u32
{
    eICW4Needed         = Bit(0),
    // Single mode if set, cascade GetMode otherwise
    eSingleMode         = Bit(1),
    // If set address interval is 4, otherwise it's 8
    eAddressInterval4   = Bit(2),
    // Level triggered mode if set, edge triggered GetMode otherwise
    eLevelTriggeredMode = Bit(3),
    // 1 means that it is Initialization Command Word
    eICW                = Bit(4),
};

enum class ICW4Flags : u32
{
    e8086Mode               = Bit(0),
    eAutoEOI                = Bit(1),
    eBufferedModeSlave      = Bit(3),
    eBufferedModeMaster     = Bit(2) | Bit(3),
    eSpecialFullyNestedMode = Bit(4),
};

ICW1Flags operator|(ICW1Flags lhs, ICW1Flags rhs)
{
    u32 ret = static_cast<u32>(lhs) | static_cast<u32>(rhs);

    return static_cast<ICW1Flags>(ret);
}

I8259A I8259A::s_Instance{};

I8259A& I8259A::Instance()
{
    return s_Instance;
}

ErrorOr<void> I8259A::Initialize()
{
    Remap(0x20, 0x28);

    return {};
}
ErrorOr<void> I8259A::Shutdown()
{
    MaskAllIRQs();

    return {};
}

ErrorOr<InterruptHandler*> I8259A::AllocateHandler(u8 hint)
{
    return IDT::AllocateHandler(hint);
}

ErrorOr<void> I8259A::Mask(u8 irq)
{
    u8 picPort = PIC1_DATA;
    if (irq >= 8)
    {
        irq -= 8;
        picPort = PIC2_DATA;
    }

    auto ocw1 = IO::In<byte>(picPort) | (1 << irq);
    IO::Out<byte>(picPort, ocw1);

    return {};
}
ErrorOr<void> I8259A::Unmask(u8 irq)
{
    u8 picPort = PIC1_DATA;
    if (irq >= 8)
    {
        irq -= 8;
        picPort = PIC2_DATA;
    }

    auto ocw1 = IO::In<byte>(picPort) & ~(1 << irq);
    IO::Out<byte>(picPort, ocw1);
    return {};
}

ErrorOr<void> I8259A::SendEOI(u8 irq)
{
    if (HandleSpuriousInterrupt(irq)) return {};

    if (irq >= 8) IO::Out<byte>(PIC2_COMMAND, OCW2_SEND_EOI);
    IO::Out<byte>(PIC1_COMMAND, OCW2_SEND_EOI);
    return {};
}

void I8259A::MaskAllIRQs()
{
    IO::Out<byte>(PIC1_DATA, 0xff);
    IO::Out<byte>(PIC2_DATA, 0xff);
}
void I8259A::UnmaskAllIRQs()
{
    IO::Out<byte>(PIC1_DATA, 0x00);
    IO::Out<byte>(PIC2_DATA, 0x00);
}

void I8259A::Remap(u8 masterOffset, u8 slaveOffset)
{
    // Save Masks
    u8   pic1Mask = IO::In<byte>(PIC1_DATA);
    u8   pic2Mask = IO::In<byte>(PIC2_DATA);

    auto icw1     = static_cast<u32>(ICW1Flags::eICW | ICW1Flags::eICW4Needed);
    // ICW1: Begin initialization sequence
    IO::Out<byte>(PIC1_COMMAND, icw1);
    IO::Wait();
    IO::Out<byte>(PIC2_COMMAND, icw1);
    IO::Wait();

    // ICW2: Set PIC offsets
    IO::Out<byte>(PIC1_DATA, masterOffset);
    IO::Wait();
    IO::Out<byte>(PIC2_DATA, slaveOffset);
    IO::Wait();

    // ICW3: Initialize Cascade Mode
    IO::Out<byte>(PIC1_DATA, 0x04);
    IO::Wait();
    IO::Out<byte>(PIC2_DATA, 0x02);
    IO::Wait();

    // ICW4: Set x86 GetMode
    auto icw4 = static_cast<u32>(ICW4Flags::e8086Mode);
    IO::Out<byte>(PIC1_DATA, icw4);
    IO::Wait();
    IO::Out<byte>(PIC2_DATA, icw4);
    IO::Wait();

    // Restore Masks
    IO::Out<byte>(PIC1_DATA, pic1Mask);
    IO::Out<byte>(PIC2_DATA, pic2Mask);
    LogInfo("PIC: Remapped to {:#x}:{:#x}", masterOffset, slaveOffset);
}

// Returns true if interrupt was spurious
bool I8259A::HandleSpuriousInterrupt(u8 irq)
{
    if (irq != 8 && irq != 15) return false;
    if (GetISR() & (1 << irq)) return false;

    if (irq == 15) IO::Out<byte>(PIC1_COMMAND, OCW2_SEND_EOI);

    return true;
}

u16 I8259A::GetIRR() { return GetIRQRegister(OCW3_GET_IRR); }
u16 I8259A::GetISR() { return GetIRQRegister(OCW3_GET_ISR); }

u16 I8259A::GetIRQRegister(u8 ocw3)
{
    IO::Out<byte>(PIC1_COMMAND, ocw3);
    IO::Out<byte>(PIC2_COMMAND, ocw3);

    return (IO::In<byte>(PIC2_COMMAND) << 8) | IO::In<byte>(PIC1_COMMAND);
}
