/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/InterruptController.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Ref.hpp>

class I8259A : public InterruptController
{
  public:
    static ::Ref<I8259A>  Instance();

    virtual ErrorOr<void> Initialize() override;
    virtual ErrorOr<void> Shutdown() override;

    virtual ErrorOr<void> Mask(u32 irq) override;
    virtual ErrorOr<void> Unmask(u32 irq) override;

    virtual ErrorOr<void> SendEOI(u32 vector) override;

    void                  MaskAllIRQs();
    void                  UnmaskAllIRQs();

    friend class ::Ref<I8259A>;

  protected:
    I8259A() = default;

    void Remap(u8 masterOffset, u8 slaveOffset);

    bool HandleSpuriousInterrupt(u8 irq);

    u16  GetIRR();
    u16  GetISR();

    u16  GetIRQRegister(u8 ocw3);
}; // namespace PIC
