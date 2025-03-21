/*
 * Created by v1tr10l7 on 21.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/PCI.hpp>

#include <Network/NetworkAdapter.hpp>

namespace RTL8139
{
    class AdapterCard : public NetworkAdapter, PCI::Device
    {
      public:
        AdapterCard(PCI::DeviceAddress& addr);

      private:
        Pointer                m_Base;
        Pointer                m_IoBase;

        constexpr static usize RECEIVE_BUFFER_SIZE  = 8192;
        constexpr static usize TRANSMIT_BUFFER_SIZE = 0x600;
        Pointer                m_ReceiveBuffer;
        Pointer                m_TransmitBuffers[4];

        enum class Register
        {
            eMac0                = 0x0000,
            eMac4                = 0x0004,
            eStatus              = 0x0008,
            eTransmitBuffer0     = 0x0020,
            eTransmitBuffer1     = 0x0024,
            eTransmitBuffer2     = 0x0028,
            eTransmitBuffer3     = 0x002c,
            eRbStart             = 0x0030,
            eCommand             = 0x0037,
            eInterruptMask       = 0x003c,
            eInterruptStatus     = 0x003e,
            eTransmitConfig      = 0x0040,
            eReceiverConfig      = 0x0044,
            eMissedPacketCounter = 0x004c,
            eCommand93c46        = 0x0050,
            eConfig0             = 0x0051,
            eConfig1             = 0x0052,
            eMediaStatus         = 0x0058,
            eBasicModeControl    = 0x0062,
            eICR                 = 0x00c0,
            eIMC                 = 0x00d8,
            eRCtl                = 0x0100,
            eTCtl                = 0x0400,
        };
        enum class ReceiverBufferLength : u32
        {
            eK8P16  = 0b00,
            eK16P16 = 0b01,
            eK32P16 = 0b10,
            eK64P16 = 0b11,
        };

        enum class DmaSize : u32
        {
            e16Bytes   = 0b000,
            e32Bytes   = 0b001,
            e64Bytes   = 0b010,
            e128Bytes  = 0b011,
            e256Bytes  = 0b100,
            e512Bytes  = 0b101,
            e1024Bytes = 0b110,
            eUnlimited = 0b111,
        };

        union ReceiveStatus
        {
            struct [[gnu::packed]]
            {
                // Receive OK
                u16 Rok       : 1;
                // Frame Alignment Error
                u16 Fae       : 1;
                // CRC Error
                u16 Crc       : 1;
                // Long Packet
                u16 Lng       : 1;
                // Runt Packet Received
                u16 Rnt       : 1;
                // Invalid Symbol Error
                u16 Ise       : 1;
                //
                u16 Reserved0 : 7;
                // Broadcast Address Received
                u16 Bar       : 1;
                // Physical Address Matched
                u16 Pam       : 1;
                // Multicast Address Received
                u16 Mar       : 1;
            };
            u16 Raw;
        };
        static_assert(sizeof(ReceiveStatus) == 2);

        union TransmitStatus
        {
            struct [[gnu::packed]]
            {
                u32 Size      : 13;
                u32 Own       : 1;
                // Transmit FIFO Underrun
                u32 Tun       : 1;
                // Transmit OK
                u32 Tok       : 1;
                // Early Tx Treeshold
                u32 ErTxTh    : 6;
                u32 Reserved0 : 2;
                // Number of Collision Count
                u32 Ncc       : 4;
                // CD Heart Beat
                u32 Cdh       : 1;
                // Out of Window Collision
                u32 Owc       : 1;
                // Transmit Abort
                u32 TAbt      : 1;
                // Carrier Sense Lost
                u32 Crc       : 1;
            };
            u32 Raw;
        };
        static_assert(sizeof(TransmitStatus) == 4);

        union Command
        {
            static constexpr auto Offset = Register::eCommand;
            struct [[gnu::packed]]
            {
                u8 BufferEmpty       : 1;
                u8 Reserved0         : 1;
                u8 TransmitterEnable : 1;
                u8 ReceiverEnable    : 1;
                u8 Reset             : 1;
                u8 Reserved1         : 3;
            };
            u8 Raw;
        };
        static_assert(sizeof(Command) == 1);

        union InterruptMask
        {
            struct [[gnu::packed]]
            {
                u16 ReceiverOk    : 1;
                u16 ReceiverError : 1;
                u16 TransmitOk    : 1;
                u16 TransmitError : 1;
                // Rx Buffer Overflow Interrupt
                u16 RxOverflow    : 1;
                u16 LinkChg       : 1;
                // Rx FIFO Overflow Interrupt
                u16 Fovw          : 1;
                u16 Reserved0     : 6;
                u16 LenChg        : 1;
                u16 TimeOut       : 1;

                u16 SystemError   : 1;
            };
            u16 Raw;
        };
        static_assert(sizeof(InterruptMask) == 2);
        union InterruptStatus
        {
            struct [[gnu::packed]]
            {
                u16 Rok         : 1;
                u16 Rer         : 1;
                u16 Tok         : 1;
                u16 Ter         : 1;
                u16 RxOvw       : 1;
                u16 LinkChg     : 1;
                u16 FOvw        : 1;
                u16 Reserved0   : 6;
                u16 Lench       : 1;
                u16 TimeOut     : 1;
                u16 SystemError : 1;
            };
            u16 Raw;
        };
        static_assert(sizeof(InterruptStatus) == 2);
        union TransmitConfiguration
        {
            struct [[gnu::packed]]
            {
                u32 ClearAbort         : 1;
                u32 Reserved0          : 3;
                u32 RetryCount         : 4;
                u32 MaxDma             : 3;
                u32 Reserved1          : 5;
                u32 AppendCrc          : 1;
                u32 LoopbackTest       : 2;
                u32 Reserved2          : 3;
                u32 HardwareVersionID2 : 2;
                u32 InterframeGapTime  : 2;
                u32 HardwareVersionID1 : 5;
                u32 Reserved3          : 1;
            };
            u32 Raw;
        };
        static_assert(sizeof(TransmitConfiguration) == 4);
        union ReceiveConfiguration
        {
            struct [[gnu::packed]]
            {
                u32 AcceptAll              : 1;
                u32 AcceptPhysicalMatch    : 1;
                u32 AcceptMulticast        : 1;
                u32 AcceptBroadcast        : 1;
                u32 AcceptRunt             : 1;
                u32 AcceptError            : 1;
                u32 Reserved0              : 1;
                u32 Wrap                   : 1;
                u32 MaxDma                 : 3;
                u32 BufferLength           : 2;
                u32 FifoThreshold          : 3;
                u32 ErrorGreater8          : 1;
                u32 MultipleEarlyInterrupt : 1;
                u32 Reserved1              : 6;
                u32 EarlyThreshold         : 4;
                u32 Reserved2              : 4;
            };
            u32 Raw;
        };
        static_assert(sizeof(ReceiveConfiguration) == 4);
        union Command93c46
        {
            struct [[gnu::packed]]
            {
                u8 Eedo          : 1;
                u8 Eedi          : 1;
                u8 Eesk          : 1;
                u8 Eecs          : 1;
                u8 Reserved0     : 2;
                u8 OperatingMode : 2;
            };
            u8 Raw;
        };
        static_assert(sizeof(Command93c46) == 1);

        union Config0
        {
            struct [[gnu::packed]]
            {
                u8 Brs : 3;
                u8 Pl  : 2;
                u8 T10 : 1;
                u8 Pcs : 1;
                u8 Scr : 1;
            };
            u8 Raw;
        };
        static_assert(sizeof(Config0) == 1);

        union Config1
        {
            struct [[gnu::packed]]
            {
                u8 PowerManagement  : 1;
                u8 VitalProductData : 1;
                u8 IoMapping        : 1;
                u8 MemoryMapping    : 1;
                u8 LWakeMode        : 1;
                u8 DriverLoad       : 1;
                u8 Leds             : 2;
            };
            u8 Raw;
        };
        static_assert(sizeof(Config1) == 1);
        union MediaStatus
        {
            struct [[gnu::packed]]
            {
                u8 ReceivePause    : 1;
                u8 TransmitPause   : 1;
                u8 LinkFailure     : 1;
                u8 Speed           : 1;
                u8 AuxPowerPresent : 1;
                u8 Reserved0       : 1;
                u8 RxFlowControl   : 1;
                u8 TxFlowControl   : 1;
            };
            u8 Raw;
        };
        static_assert(sizeof(MediaStatus) == 1);

        union BasicModeControl
        {
            struct [[gnu::packed]]
            {
                u16 Reserved0              : 8;
                u16 DuplexMode             : 1;
                u16 RestartAutoNegotiation : 1;
                u16 Reserved1              : 2;
                u16 AutoNegotiationEnable  : 1;
                u16 Speed100Mbps           : 1;
                u16 Reserved2              : 1;
                u16 Reset                  : 1;
            };
            u16 Raw;
        };
        static_assert(sizeof(BasicModeControl) == 2);

        template <typename T>
        inline T Read(Register reg) const
        {
            Pointer regAddr = m_Base.ToHigherHalf<Pointer>().Offset<Pointer>(
                std::to_underlying(reg));

            return *regAddr.As<T>();
        }
        template <typename T>
        inline void Write(Register reg, T data)
        {
            Pointer regAddr = m_Base.ToHigherHalf<Pointer>().Offset<Pointer>(
                std::to_underlying(reg));
            *regAddr.As<T>() = data;
        }

        inline u32 ReadReg(Register reg) const
        {
            return *m_Base.ToHigherHalf<Pointer>()
                        .Offset<Pointer>(std::to_underlying(reg))
                        .As<u32>();
        }
        inline void WriteReg(Register reg, u32 data) const
        {
            *m_Base.ToHigherHalf<Pointer>()
                 .Offset<Pointer>(std::to_underlying(reg))
                 .As<u32>()
                = data;
        }
    };
} // namespace RTL8139
