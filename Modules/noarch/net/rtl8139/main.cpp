/*
 * Created by v1tr10l7 on 21.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "RTL8139.hpp"

#include <Arch/CPU.hpp>
#include <Library/Module.hpp>

namespace RTL8139
{
    static std::array s_IdTable = {
        PCI::DeviceID{0x10ec, 0x8139, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x10ec, 0x8138, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x1113, 0x1211, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x1500, 0x1360, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x4033, 0x1360, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x1186, 0x1300, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x1186, 0x1340, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x13d1, 0xab06, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x1259, 0xa117, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x1259, 0xa11e, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x14ea, 0xab06, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x14ea, 0xab07, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x11db, 0x1234, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x1432, 0x9130, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x02ac, 0x1012, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x018a, 0x0106, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x126c, 0x1211, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x1743, 0x8139, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x021b, 0x8139, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{0x16ec, 0xab06, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},

#ifdef CONFIG_SH_SECUREEDGE5410
        /* Bogus 8139 silicon reports 8129 without external PROM :-( */
        PCI::DeviceID{0x10ec, 0x8129, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0},
#endif
#ifdef CONFIG_8139TOO_8129
        PCI::DeviceID{0x10ec, 0x8129, PCI::DeviceID::ANY_ID,
                      PCI::DeviceID::ANY_ID, 2, 0, RTL8129},
#endif

        /* some crazy cards report invalid vendor ids like
         * 0x0001 here.  The other ids are valid and constant,
         * so we simply don't match on the main vendor id.
         */
        PCI::DeviceID{PCI::DeviceID::ANY_ID, 0x8139, 0x10ec, 0x8139, 2, 0},
        PCI::DeviceID{PCI::DeviceID::ANY_ID, 0x8139, 0x1186, 0x1300, 2, 0},
        PCI::DeviceID{PCI::DeviceID::ANY_ID, 0x8139, 0x13d1, 0xab06, 2, 0},
    };

    bool AdapterCard::Send(u8* data, usize length)
    {
        usize bufferIndex = 0;
        for (usize i = 0; i < 4; i++)
        {
            auto potentialBuffer = (m_TransmitNext + i) % 4;
            auto status          = Read<TransmitStatus>(static_cast<Register>(
                potentialBuffer
                + std::to_underlying(Register::eTransmitStatus0)));
            if (status.Own == 1)
            {
                bufferIndex = potentialBuffer;
                goto skip;
            }
        }
        return false;

    skip:
        auto& transmit = m_TransmitBuffers[bufferIndex];
        m_TransmitNext = (bufferIndex + 1) % 4;

        std::memcpy(transmit.As<void>(), data, length);
        if (auto left = TRANSMIT_BUFFER_SIZE - length; left > 0)
            std::memset(transmit.Offset<Pointer>(length).As<void*>(), 0, left);

        Register transmitStatusReg = static_cast<Register>(
            bufferIndex + std::to_underlying(Register::eTransmitStatus0));
        auto status = Read<TransmitStatus>(transmitStatusReg);
        status.Size = length;
        status.Own  = 0;
        Write<TransmitStatus>(transmitStatusReg, status);

        return true;
    }

    AdapterCard::AdapterCard(PCI::DeviceAddress& addr)
        : PCI::Device(addr)
    {
        EnableBusMastering();
        EnableMemorySpace();

        PCI::Bar bar0 = GetBar(0);
        PCI::Bar bar1 = GetBar(1);
        PCI::Bar bar2 = GetBar(2);
        PCI::Bar bar3 = GetBar(3);
        PCI::Bar bar4 = GetBar(4);
        PCI::Bar bar5 = GetBar(5);

        if (bar0.IsMMIO) m_Base = bar0.Address.ToHigherHalf<Pointer>();
        else if (bar1.IsMMIO) m_Base = bar1.Address.ToHigherHalf<Pointer>();
        else if (bar2.IsMMIO) m_Base = bar2.Address.ToHigherHalf<Pointer>();
        else if (bar3.IsMMIO) m_Base = bar3.Address.ToHigherHalf<Pointer>();
        else if (bar4.IsMMIO) m_Base = bar4.Address.ToHigherHalf<Pointer>();
        else if (bar5.IsMMIO) m_Base = bar5.Address.ToHigherHalf<Pointer>();
        m_IoBase
            = bar1.IsMMIO ? bar1.Address.ToHigherHalf<Pointer>() : bar2.Address;

        auto command  = Read<Command>(Register::eCommand);
        command.Reset = true;
        Write(Register::eCommand, command);

        while (Read<Command>(Register::eCommand).Reset) Arch::Pause();
        auto command93c46 = Read<Command93c46>(Register::eCommand93c46);
        command93c46.OperatingMode = 0b11;
        Write(Register::eCommand93c46, command93c46);

        auto config1      = Read<Config1>(Register::eConfig1);
        config1.LWakeMode = false;
        Write<Config1>(Register::eConfig1, config1);

        command                   = Read<Command>(Register::eCommand);
        command.TransmitterEnable = true;
        command.ReceiverEnable    = true;
        Write<Command>(Register::eCommand, command);

        m_ReceiveBuffer = new u8[RECEIVE_BUFFER_SIZE + 16 + 1500];
        Write<u32>(Register::eRbStart, m_ReceiveBuffer.FromHigherHalf<u64>());
        Write<u8>(Register::eMissedPacketCounter, 0);

        auto basicModeControl
            = Read<BasicModeControl>(Register::eBasicModeControl);
        basicModeControl.Speed100Mbps          = true;
        basicModeControl.DuplexMode            = true;
        basicModeControl.AutoNegotiationEnable = true;
        Write<BasicModeControl>(Register::eBasicModeControl, basicModeControl);

        auto mediaStatus          = Read<MediaStatus>(Register::eMediaStatus);
        mediaStatus.RxFlowControl = true;
        Write<MediaStatus>(Register::eMediaStatus, mediaStatus);

        auto receiverConfig
            = Read<ReceiveConfiguration>(Register::eReceiverConfig);
        receiverConfig.AcceptAll           = true;
        receiverConfig.AcceptPhysicalMatch = true;
        receiverConfig.AcceptMulticast     = true;
        receiverConfig.AcceptBroadcast     = true;
        receiverConfig.Wrap                = true;
        receiverConfig.MaxDma = std::to_underlying(DmaSize::e1024Bytes);
        receiverConfig.BufferLength
            = std::to_underlying(ReceiverBufferLength::eK8P16);
        receiverConfig.EarlyThreshold = 0b111;
        Write<ReceiveConfiguration>(Register::eReceiverConfig, receiverConfig);

        auto transmitConfig
            = Read<TransmitConfiguration>(Register::eTransmitConfig);
        transmitConfig.RetryCount = 0;
        transmitConfig.MaxDma     = std::to_underlying(DmaSize::e1024Bytes);
        transmitConfig.InterframeGapTime = 0b11;
        Write<TransmitConfiguration>(Register::eTransmitConfig, transmitConfig);

        for (usize i = 0; auto& buffer : m_TransmitBuffers)
        {
            buffer = new u8[TRANSMIT_BUFFER_SIZE];
            usize bufferRegister
                = std::to_underlying(Register::eTransmitBuffer0) + i;

            Write<u32>(
                static_cast<Register>(bufferRegister),
                reinterpret_cast<uintptr_t>(buffer.FromHigherHalf<u64>()));
        }
        command93c46 = Read<Command93c46>(Register::eCommand93c46);
        command93c46.OperatingMode = 0b00;
        Write<Command93c46>(Register::eCommand93c46, command93c46);

        command                   = Read<Command>(Register::eCommand);
        command.TransmitterEnable = true;
        command.ReceiverEnable    = true;
        Write<Command>(Register::eCommand, command);

        auto mac0          = Read<u32>(Register::eMac0);
        auto mac4          = Read<u32>(Register::eMac4);

        m_MacAddress[0]    = mac0 & 0xFF;
        m_MacAddress[1]    = (mac0 >> 8) & 0xFF;
        m_MacAddress[2]    = (mac0 >> 16) & 0xFF;
        m_MacAddress[3]    = (mac0 >> 24) & 0xFF;
        m_MacAddress[4]    = mac4 & 0xFF;
        m_MacAddress[5]    = (mac4 >> 8) & 0xFF;

        auto interruptMask = Read<InterruptMask>(Register::eInterruptMask);
        interruptMask.ReceiverOk    = true;
        interruptMask.ReceiverError = true;
        interruptMask.TransmitOk    = true;
        interruptMask.TransmitError = true;
        interruptMask.RxOverflow    = true;
        interruptMask.LinkChg       = true;
        interruptMask.Fovw          = true;
        interruptMask.TimeOut       = true;
        interruptMask.SystemError   = true;
        Write<InterruptMask>(Register::eInterruptMask, interruptMask);

        Delegate<void()> delegate;
        delegate.BindLambda([&]() { HandleInterrupt(); });
        if (!RegisterIrq(CPU::GetCurrent()->LapicID, delegate))
            LogError("RTL8139: Failed to register interrupt handler");

        InterruptStatus ack;
        ack.Rok         = true;
        ack.Rer         = true;
        ack.Tok         = true;
        ack.Ter         = true;
        ack.RxOvw       = true;
        ack.LinkChg     = true;
        ack.FOvw        = true;
        ack.TimeOut     = true;
        ack.SystemError = true;
        Write<InterruptStatus>(Register::eInterruptStatus, ack);

        SendCommand(Bit(10), false);
        LogInfo("RTL8139: MAC address: {}", m_MacAddress);
    }

    ErrorOr<void> ProbeDevice(PCI::DeviceAddress&  address,
                              const PCI::DeviceID& id)
    {
        LogTrace("RTL8139: Detected pci nic device");
        auto nic = new AdapterCard(address);

        if (!NetworkAdapter::RegisterNIC(nic))
        {
            LogError("RTL8139: Failed to register nic device");
            return Error(ENODEV);
        }
        return {};
    }
    void               RemoveDevice(PCI::Device& device) {}

    static PCI::Driver s_Driver = {
        .Name     = "rtl8139",
        .MatchIDs = std::span(s_IdTable.begin(), s_IdTable.end()),
        .Probe    = ProbeDevice,
        .Remove   = RemoveDevice,
    };

    bool ModuleInit() { return PCI::RegisterDriver(s_Driver); }

    MODULE_INIT(rtl8139, ModuleInit);
}; // namespace RTL8139
