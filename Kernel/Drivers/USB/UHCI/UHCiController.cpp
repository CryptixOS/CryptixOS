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

namespace USB::UHCI
{
    ErrorOr<void> Controller::Initialize()
    {
        LogTrace("USB: Initializing the UHCI Controller...");
        LogTrace("UHCI: Acquiring PCI Bar4...");
        m_Bar = GetBar(4);
        m_IoRegisters.Initialize(m_Bar.Base, m_Bar.Size, IoSpace::eSystemIO);

        if (m_Bar.Base)
            LogInfo(
                "UHCI: BAR4 => {{ .Base: {:#x}, .Address: {:#x}, .Size: {:#x}, "
                ".IsMMIO: {:b}, .DisableCache: {:b}, .Is64Bit: {:b} }}",
                m_Bar.Base, m_Bar.Address, m_Bar.Size, m_Bar.IsMMIO,
                m_Bar.DisableCache, m_Bar.Is64Bit);

#ifdef CTOS_TARGET_X86_64
        // Disable Legacy USB emulation
        ::IO::Out<word>(m_Bar.Base.Offset(0xc0), 0x2000);
        LogTrace("UHCI: Legacy USB emulation disabled.");
#endif

        LogTrace("UHCI: Enabling bus mastering...");
        EnableBusMastering();

        LogTrace("UHCI: Resetting the host controller...");
        auto status = Reset();
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
        if (!RegisterIrq(CPU::Current()->LapicID, delegate))
        {
            LogError("UHCI: Failed to register interrupt handler");
            return Error(EBUSY);
        }

        LogInfo("UHCI: Successfully registered an interrupt handler");
        m_OnIrq.BindLambda([&]() { HandleInterrupt(); });

        LogTrace("UHCI: MsiSupported => {}, MsiOffset => {:#x},\nMsixSupported => {}, MsixOffset => {:#x}",
                m_MsiSupported, m_MsiOffset, m_MsixSupported, m_MsixOffset);

        // TODO(v1tr10l7): Create Root Hub

        SendCommand(Bit(10), false);
        return {};
    };

    ErrorOr<void> Controller::Start()
    {
        // Reset
        Write(Register::eCommand, Command::eHostControllerReset);

        // FIXME(v1tr10l7): Timeout?
        while (Read(Register::eCommand) & Command::eHostControllerReset)
            IO::Delay(50);

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
        while ((Read(Register::eStatus) & Status::eHalted) == 0)
            IO::Delay(1000);

        LogTrace("UHCI: Controller halted successfully");
        return {};
    }

    ErrorOr<void> Controller::Reset()
    {
        Write(Register::eCommand, Command::eHostControllerReset);
        IO::Delay(50);
        Write(Register::eCommand, 0);
        IO::Delay(10);

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

    i32  Controller::IoCtl(usize request, uintptr_t argp) { return -1; }

    void Controller::HandleInterrupt()
    {
        LogTrace("UHCI: Handling interrupt...");

        LogTrace("UHCI: Leaving interrupt...");
    }
}; // namespace USB::UHCI
