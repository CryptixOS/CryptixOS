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

#include "Scheduler/Process.hpp"
#include "Scheduler/Scheduler.hpp"
#include "Scheduler/Thread.hpp"

#include "Utility/ICxxAbi.hpp"
#include "Utility/Stacktrace.hpp"

void kernelThread()
{
    for (;;) LogInfo("Hello");
}

extern "C" __attribute__((no_sanitize("address"))) void kernelStart()
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

    static Process* kernelProcess = new Process("Kernel Process");
    kernelProcess->pageMap        = VMM::GetKernelPageMap();
    Scheduler::EnqueueThread(new Thread(
        kernelProcess, reinterpret_cast<uintptr_t>(kernelThread), false));

    Scheduler::Initialize();

    for (;;) Arch::Halt();
}
