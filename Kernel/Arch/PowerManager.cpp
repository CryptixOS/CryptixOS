/*
 * Created by v1tr10l7 on 20.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/PowerManager.hpp>
#include <Boot/BootInfo.hpp>
#ifdef CTOS_TARGET_X86_64
    #include <Arch/x86_64/IO.hpp>
#endif

#include <Drivers/Terminal.hpp>
#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/EFI/SystemTable.hpp>

#include <Memory/VMM.hpp>

namespace PowerManager
{
    void Reboot()
    {
        Terminal* terminal = Terminal::GetPrimary();

        terminal->Clear();
        auto print = [terminal](const char* string)
        {
            LogTrace("{}", string);
            terminal->PrintString(string);
        };

        print("PowerManager: Restarting system...\n");
        print("PowerManager: Trying to reboot via ACPI...");
        ACPI::Reboot();
        print("PowerManager: Rebooting via ACPI was unsuccessful");

        print("PowerManager: Trying to reboot via EFI Runtime Services...");
        EFI::SystemTable* systemTable
            = BootInfo::GetEfiSystemTable().As<EFI::SystemTable>();

        Assert(systemTable);
        systemTable
            = VMM::MapIoRegion<EFI::SystemTable>(Pointer(systemTable), true);

        auto runtimeServices = VMM::MapIoRegion<EFI::RuntimeServices>(
            Pointer(systemTable->RuntimeServices), true);
        runtimeServices->ResetSystem(EFI::ResetType::eShutdown, 0, 0, nullptr);

        // TODO(v1tr10l7): UEFI Service
        print(
            "PowerManager: Rebooting via EFI Runtime Services was "
            "unsuccessful");
        Arch::Reboot();
    }
    void PowerOff()
    {
#ifdef CTOS_TARGET_X86_64
        // Qemu shutdown
        IO::Out<u16>(0x604, 0x2000);
        IO::Out<u16>(0xb004, 0x2000);

        // Virtualbox shutdown
        IO::Out<u16>(0x4004, 0x3400);
#endif
    }
}; // namespace PowerManager
