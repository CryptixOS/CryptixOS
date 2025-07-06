/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

namespace USB::UHCI
{
    struct [[gnu::packed]] FrameListEntry
    {
        bool Empty           : 1;
        bool QueueHead       : 1;
        u8   Reserved        : 2;
        u32  PhysicalAddress : 28;
    };
    struct [[gnu::packed]] QueueHead
    {
        u32 HorizontalPointer = 0;
        u32 VerticalPointer   = 0;
    };
    struct [[gnu::packed]] TransferDescriptor
    {
        u32 NextDescriptor = 0;
        u32 Status         = 0;
        u32 PacketHeader   = 0;
        u32 BufferAddress  = 0;
        u32 SystemUse      = 0;
    };
    struct [[gnu::packed]] TransferDescriptorNextDescriptor
    {
        u32  PhysicalAddress : 28;
        bool Reserved       : 1;
        bool DepthFirst     : 1;
        bool QueueHead      : 1;
        bool Terminate      : 1;
    };
    struct [[gnu::packed]] TransferDescriptorStatus 
    {
        u8 Reserved : 2;
        bool ShortPacketDetect : 1;
        u8 ErrorCounter : 2;
        bool LowSpeed : 1;
        bool IsIsochronous : 1;
        bool InterruptOnComplete : 1;
        bool Active : 1;
        bool Stalled : 1;
        bool DataBufferError : 1;
        bool BabbleDetected  : 1;
        bool NonAcknowledged : 1;
        bool TimeoutCRC : 1;
        bool BitStuffError : 1;
        u16 Reserved2 : 6;
        u16 ActualLength : 11;
    };
    struct [[gnu::packed]] TransferDescriptorPacketHeader 
    {
        u16 MaximumLength : 11;
        bool Reserved : 1;
        bool DataToggle : 1;
        u8 Endpoint : 4;
        u8 Device : 7;
        u8 PacketType;
    };
}; // namespace USB::UHCI
