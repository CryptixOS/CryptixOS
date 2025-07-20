/*
 * Created by v1tr10l7 on 06.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

namespace PCI
{
    struct [[gnu::packed]] MsiControl
    {
        u16 MsiE     : 1;
        u16 Mmc      : 3;
        u16 Mme      : 3;
        u16 C64      : 1;
        u16 PVM      : 1;
        u16 Reserved : 6;
    };
    struct [[gnu::packed]] MsiAddress
    {
        u32 Reserved0       : 2;
        u32 DestinationMode : 1;
        u32 RedirectionHint : 1;
        u32 Reserved1       : 8;
        u32 DestinationID   : 8;
        u32 BaseAddress     : 12;
    };
    struct [[gnu::packed]] MsiData
    {
        u32 Vector       : 8;
        u32 DeliveryMode : 3;
        u32 Reserved0    : 3;
        u32 Level        : 1;
        u32 TriggerMode  : 1;
        u32 Reserved1    : 16;
    };

    struct [[gnu::packed]] MsiXControl
    {
        u16 Irqs     : 11;
        u16 Reserved : 3;
        u16 Mask     : 1;
        u16 Enable   : 1;
    };
    struct [[gnu::packed]] MsiXAddress
    {
        u32 Bir    : 3;
        u32 Offset : 29;
    };
    struct [[gnu::packed]] MsiXVectorCtrl
    {
        u32 Mask     : 8;
        u32 Reserved : 31;
    };
    struct [[gnu::packed]] MsiXEntry
    {
        u32 AddressLow;
        u32 AddressHigh;
        u32 Data;
        u32 Control;
    };
}; // namespace PCI
