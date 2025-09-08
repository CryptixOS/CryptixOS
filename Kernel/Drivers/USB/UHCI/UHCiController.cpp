/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/USB/UHCI/UHCiController.hpp>
#include <Library/Logger.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

static constexpr u8 MAXIMUM_NUMBER_OF_TDS
    = 128; // Upper pool limit. This consumes the second page we have allocated
CTOS_UNUSED static constexpr u8  MAXIMUM_NUMBER_OF_QHS          = 64;
CTOS_UNUSED static constexpr u8  RETRY_COUNTER_RELOAD           = 3;

CTOS_UNUSED static constexpr u8  UHCI_NUMBER_OF_ISOCHRONOUS_TDS = 128;
CTOS_UNUSED static constexpr u16 UHCI_NUMBER_OF_FRAMES          = 1024;

namespace USB::UHCI
{
    ErrorOr<void> Controller::Initialize()
    {
        LogTrace("USB: Initializing the UHCI Controller...");
        auto status = Enable();
        if (!status)
        {
            LogError("UHCI: Failed to enable the device");
            return Error(status.error());
        }

        LogTrace("UHCI: Acquiring PCI Bar4...");
        m_Bar = GetBar(4);

#ifdef CTOS_TARGET_X86_64
        m_IoRegisters.Initialize(m_Bar.Base, m_Bar.Size, IoSpace::eSystemIO);
#else
        LogError("UHCI: SystemIO is not supported on this platform");
#endif

        if (m_Bar.Base)
            LogInfo(
                "UHCI: BAR4 => {{ .Base: {:#x}, .Address: {:#x}, .Size: {:#x}, "
                ".IsMMIO: {:b}, .DisableCache: {:b}, .Is64Bit: {:b} }}",
                m_Bar.Base, m_Bar.Address, m_Bar.Size, m_Bar.IsMMIO,
                m_Bar.DisableCache, m_Bar.Is64Bit);

#ifdef CTOS_TARGET_X86_64
        // Disable Legacy USB emulation
        ::IO::Out<u16>(m_Bar.Base.Offset(0xc0), 0x2000);
        LogTrace("UHCI: Legacy USB emulation disabled.");
#endif

        LogTrace("UHCI: Enabling bus mastering...");
        EnableBusMastering();

        LogTrace("UHCI: Resetting the host controller...");
        status = Reset();
        RetOnError(status);

        Pointer framelistPhys = PMM::CallocatePages(1);
        m_FrameList
            = VMM::MapIoRegion(framelistPhys, PMM::PAGE_SIZE, true, 4_kib);
        for (usize i = 0; i < 1024; i++) m_FrameList[i].Empty = true;

        // Detect ports
        usize port = 0;
        for (port = 0; port < (m_Bar.Size - 0x10) / 2; port++)
        {
            u32 status = Read(static_cast<Register>(0x10 + port * 2));
            if ((status & 0x80) == 0) break;
        }

        LogTrace("UHCI: Detected {} ports", port);
        // TODO(v1tr10l7): Allocate td, and qd

        status = Start();
        if (!status)
        {
            LogError("UHCI: Failed to start the controller...");
            return Error(ENOSPC);
        }

        Delegate<void()> delegate;
        delegate.BindLambda([&]() { HandleInterrupt(); });

        LogTrace("UHCI: Registering the interrupt handler...");
#ifdef CTOS_TARGET_X86_64
        if (!RegisterIrq(CPU::Current()->LapicID, delegate))
        {
            LogError("UHCI: Failed to register interrupt handler");
            return Error(EBUSY);
        }
#else
        LogError("UHCI: Failed to register interrupt handler");
        return Error(EBUSY);
#endif

        LogInfo("UHCI: Successfully registered an interrupt handler");
        m_OnIrq.BindLambda([&]() { HandleInterrupt(); });

        LogTrace(
            "UHCI: MsiSupported => {}, MsiOffset => {:#x},\nMsixSupported => "
            "{}, MsixOffset => {:#x}",
            m_MsiSupported, m_MsiOffset, m_MsixSupported, m_MsixOffset);

        // TODO(v1tr10l7): Create Root Hub

        EnableInterrupts();
        m_QhPool = PMM::CallocatePages(2);
        m_QhPool = m_QhPool.ToHigherHalf();

        m_FreeQhPool.Resize(MAXIMUM_NUMBER_OF_TDS);
        for (usize i = 0; i < m_FreeQhPool.Size(); i++)
        {
            auto placement = m_QhPool.Offset<void*>(i * sizeof(QueueHead));
            auto phys      = m_QhPool.FromHigherHalf<Pointer>().Offset<u32>(
                (i * sizeof(QueueHead)));
            m_FreeQhPool[i] = new (placement) QueueHead(phys);
        }

        // Create the Full Speed, Low Speed Control and Bulk Queue Heads
        m_InterruptTransferQueue = AllocateQueueHead();
        m_LowSpeedControlQh      = AllocateQueueHead();
        m_FullSpeedControlQh     = AllocateQueueHead();
        m_BulkQh                 = AllocateQueueHead();
        m_DummyQh                = AllocateQueueHead();

        Pointer tdPool           = PMM::CallocatePages(2);
        m_TdPool                 = tdPool.ToHigherHalf();

        // Set up the Isochronous Transfer Descriptor list
        m_IsoTdList.Resize(UHCI_NUMBER_OF_ISOCHRONOUS_TDS);
        for (usize i = 0; i < m_IsoTdList.Size(); i++)
        {
            auto placement
                = m_TdPool.Offset<void*>((i * sizeof(TransferDescriptor)));
            auto phys = m_TdPool.FromHigherHalf<Pointer>().Offset<u32>(
                (i * sizeof(TransferDescriptor)));

            // Place a new Transfer Descriptor with a 1:1 in our region
            // The pointer returned by `new()` lines up exactly with the value
            // that we store in `paddr`, meaning our member functions directly
            // access the raw descriptor (that we later send to the controller)
            m_IsoTdList[i]          = new (placement) TransferDescriptor(phys);
            auto transferDescriptor = m_IsoTdList[i];
            transferDescriptor->SetInUse(
                true); // Isochronous transfers are ALWAYS marked as in use
            //     (in
            // case we somehow get allocated one...)
            transferDescriptor->SetIsochronous();
            transferDescriptor->LinkQueueHead(m_InterruptTransferQueue->Phys());
        }

        m_FreeTdPool.Resize(MAXIMUM_NUMBER_OF_TDS);
        for (usize i = 0; i < m_FreeTdPool.Size(); i++)
        {
            auto placement
                = m_TdPool.Offset<Pointer>(PMM::PAGE_SIZE)
                      .Offset<void*>((i * sizeof(TransferDescriptor)));
            auto phys = m_TdPool.FromHigherHalf<Pointer>().Offset<u32>(
                i * sizeof(TransferDescriptor));

            // Place a new Transfer Descriptor with a 1:1 in our region
            // The pointer returned by `new()` lines up exactly with the value
            // that we store in `paddr`, meaning our member functions directly
            // access the raw descriptor (that we later send to the controller)
            m_FreeTdPool[i] = new (placement) TransferDescriptor(phys);
        }

        m_InterruptTransferQueue->LinkNextQueueHead(m_LowSpeedControlQh);
        m_InterruptTransferQueue->TerminateElementLink();

        m_LowSpeedControlQh->LinkNextQueueHead(m_FullSpeedControlQh);
        m_LowSpeedControlQh->TerminateElementLink();

        m_FullSpeedControlQh->LinkNextQueueHead(m_BulkQh);
        m_FullSpeedControlQh->TerminateElementLink();

        m_BulkQh->LinkNextQueueHead(m_DummyQh);
        m_BulkQh->TerminateElementLink();

        auto piix4_td_hack = AllocateTransferDescriptor();
        piix4_td_hack->Terminate();
        piix4_td_hack->SetMaxLen(0x7ff); // Null data packet
        piix4_td_hack->SetDeviceAddress(0x7f);
        piix4_td_hack->SetPacketID(PacketID::IN);
        m_DummyQh->TerminateWithStrayDescriptor(piix4_td_hack);
        m_DummyQh->TerminateElementLink();

        u32* framelist = reinterpret_cast<u32*>(m_FrameList);
        for (int frame = 0; frame < UHCI_NUMBER_OF_FRAMES; frame++)
        {
            // Each frame pointer points to iso_td % NUM_ISO_TDS
            framelist[frame]
                = m_IsoTdList.At(frame % UHCI_NUMBER_OF_ISOCHRONOUS_TDS)
                      ->Phys();
        }

        Write(Register::eStartOfFrameModify, 64);

        Write(Register::eFrameListBaseAddress, framelistPhys);
        Write(Register::eFrameNumber, 0);

        Write(Register::eInterruptEnable, false);

        return {};
    };

    ErrorOr<void> Controller::Start()
    {
        // Reset
        Write(Register::eCommand, Command::eHostControllerReset);

        // FIXME(v1tr10l7): Timeout?
#ifdef CTOS_TARGET_X86_64
        while (Read(Register::eCommand) & Command::eHostControllerReset)
            IO::Delay(50);
#endif

        // Enable interrupts
        Write(Register::eInterruptEnable,
              InterruptEnable::eTimeOutCRC | InterruptEnable::eResume
                  | InterruptEnable::eCompleteTransfer
                  | InterruptEnable::eShortPacket);

        Write(Register::eFrameNumber, 0);
        Write(Register::eFrameListBaseAddress, Pointer(m_FrameList));

        Write(Register::eCommand, Command::eRun | Command::eConfigureFlag
                                      | Command::e64BitPacketSize);
        return {};
    }
    ErrorOr<void> Controller::Stop()
    {
        LogTrace("UHCI: Stopping the controller...");
        auto command = Read(Register::eCommand);

        Write(Register::eCommand, command & ~Command::eRun);
#ifdef CTOS_TARGET_X86_64
        while ((Read(Register::eStatus) & Status::eHalted) == 0)
            IO::Delay(1000);
#endif

        LogTrace("UHCI: Controller halted successfully");
        return {};
    }

    ErrorOr<void> Controller::Reset()
    {
        Write(Register::eCommand, Command::eHostControllerReset);
#ifdef CTOS_TARGET_X86_64
        IO::Delay(50);
#endif
        Write(Register::eCommand, 0);
#ifdef CTOS_TARGET_X86_64
        IO::Delay(10);
#endif

        /*
        auto status = Stop();
        RetOnError(status);

        Write(Register::eCommand, Command::eHostControllerReset);

        while ((Read(Register::eCommand) & Command::eHostControllerReset) != 0)
            IO::Delay(1000);

        Write(Register::eStartOfFrameModify, 64);

        Write(Register::eFrameListBaseAddress, framelistPhys);
        Write(Register::eFrameNumber, 0);

        Write(Register::eInterruptEnable, false);
        */
        LogTrace("UHCI: Reset completed");
        return {};
    }

    ErrorOr<isize> Controller::Read(const UserBuffer& out, usize count,
                                    isize offset)
    {
        return Error(ENOSYS);
    }

    ErrorOr<isize> Controller::Read(void* dest, off_t offset, usize bytes)
    {
        return Error(ENOSYS);
    }
    ErrorOr<isize> Controller::Write(const void* src, off_t offset, usize bytes)
    {
        return Error(ENOSYS);
    }

    ErrorOr<isize> Controller::Write(const UserBuffer& in, usize count,
                                     isize offset)
    {
        return Error(ENOSYS);
    }

    i32        Controller::IoCtl(usize request, uintptr_t argp) { return -1; }

    QueueHead* Controller::AllocateQueueHead()
    {
        for (QueueHead* queueHead : m_FreeQhPool)
        {
            if (!queueHead->InUse())
            {
                queueHead->SetInUse(true);
                return queueHead;
            }
        }

        return nullptr; // Huh!? We're outta queue heads!
    }
    TransferDescriptor* Controller::AllocateTransferDescriptor() const
    {
        for (TransferDescriptor* transferDescriptor : m_FreeTdPool)
        {
            if (!transferDescriptor->InUse())
            {
                transferDescriptor->SetInUse(true);
                return transferDescriptor;
            }
        }

        return nullptr; // Huh?! We're outta TDs!!
    }

    void Controller::HandleInterrupt()
    {
        LogTrace("UHCI: Handling interrupt...");

        LogTrace("UHCI: Leaving interrupt...");
    }
}; // namespace USB::UHCI
