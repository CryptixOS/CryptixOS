/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Prism/Types.hpp"

namespace PIC
{
    void Remap(u8 masterOffset, u8 slaveOffset);

    void MaskIRQ(u8 irq);
    void UnmaskIRQ(u8 irq);
    void MaskAllIRQs();
    void UnmaskAllIRQs();

    void SendEOI(u8 irq);

    u16  GetIRR();
    u16  GetISR();
}; // namespace PIC
