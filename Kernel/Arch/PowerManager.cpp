/*
 * Created by v1tr10l7 on 20.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/PowerManager.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Drivers/Terminal.hpp>
#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/EFI/SystemTable.hpp>

namespace PowerManager
{
    void Reboot()
    {
        Terminal* terminal = Terminal::GetPrimary();

        terminal->Clear();

        terminal->PrintString("Restarting system...\n");

        EFI::SystemTable* systemTable
            = BootInfo::GetEfiSystemTable().As<EFI::SystemTable>();
        Assert(systemTable);

        // systemTable->RuntimeServices->ResetSystem(
        //     EFI::ResetType::eResetShutdown, 0, 0, nullptr);

        ACPI::Reboot();
        // TODO(v1tr10l7): UEFI Service
        Arch::Reboot();
    }
    void Shutdown()
    {
        // Qemu shutdown
        IO::Out<u16>(0x604, 0x2000);
        IO::Out<u16>(0xb004, 0x2000);

        // Virtualbox shutdown
        IO::Out<u16>(0x4004, 0x3400);
    }

}; // namespace PowerManager
