/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/InterruptController.hpp>
#include <Prism/Core/Types.hpp>

class I8259A : public InterruptController
{
  public:
    static I8259A& Instance();

    virtual ErrorOr<void> Initialize() override;
    virtual ErrorOr<void> Shutdown() override;

    virtual ErrorOr<InterruptHandler*> AllocateHandler(u8 hint = 0x20 + 0x10) override;
    
    virtual ErrorOr<void> Mask(u8 irq) override;
    virtual ErrorOr<void> Unmask(u8 irq) override;

    virtual ErrorOr<void> SendEOI(u8 vector) override;

    void MaskAllIRQs();
    void UnmaskAllIRQs();

  private:
    I8259A() = default;
    static I8259A s_Instance;

    void Remap(u8 masterOffset, u8 slaveOffset);

    bool HandleSpuriousInterrupt(u8 irq);

    u16  GetIRR();
    u16  GetISR();

    u16 GetIRQRegister(u8 ocw3);
}; // namespace PIC
