/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include "Common.hpp"

#include "ACPI/ACPI.hpp"
#include "Arch/InterruptManager.hpp"

#include "Drivers/Serial.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"

#include "Utility/ICxxAbi.hpp"
#include "Utility/Stacktrace.hpp"

extern "C" void kernelStart()
{
    Logger::EnableOutput(LOG_OUTPUT_TERMINAL);
    InterruptManager::InstallExceptionHandlers();

    Assert(PMM::Initialize());
    icxxabi::Initialize();

    VMM::Initialize();
    Serial::Initialize();
    Logger::EnableOutput(LOG_OUTPUT_SERIAL);

    LogInfo("Boot: Kernel loaded with {}-{} -> boot time: {}s",
            BootInfo::GetBootloaderName(), BootInfo::GetBootloaderVersion(),
            BootInfo::GetBootTime());

    LogInfo("Boot: Kernel Physical Address: {:#x}",
            BootInfo::GetKernelPhysicalAddress());
    LogInfo("Boot: Kernel Virtual Address: {:#x}",
            BootInfo::GetKernelVirtualAddress());
    Stacktrace::Initialize();
    ACPI::Initialize();
    Arch::Initialize();

    for (;;) Arch::Halt();
}
