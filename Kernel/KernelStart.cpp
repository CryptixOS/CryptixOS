/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include "Common.hpp"

#include "Drivers/Serial.hpp"

#include "Memory/PMM.hpp"

#include "Utility/ICxxAbi.hpp"
#include "Utility/Stacktrace.hpp"

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include "Arch/x86_64/GDT.hpp"
    #include "Arch/x86_64/IDT.hpp"
#endif

extern "C" void kernelStart()
{
    Serial::Initialize();
    Logger::EnableOutput(LOG_OUTPUT_SERIAL);

    Logger::EnableOutput(LOG_OUTPUT_TERMINAL);

#if CTOS_ARCH == CTOS_ARCH_X86_64
    GDT::Initialize();
    GDT::Load(0);

    IDT::Initialize();
    IDT::Load();
#endif

    Assert(PMM::Initialize());
    icxxabi::Initialize();

    LogInfo("Boot: Kernel loaded with {}-{} -> boot time: {}s",
            BootInfo::GetBootloaderName(), BootInfo::GetBootloaderVersion(),
            BootInfo::GetBootTime());

    LogInfo("Boot: Kernel Physical Address: {:#x}",
            BootInfo::GetKernelPhysicalAddress());
    LogInfo("Boot: Kernel Virtual Address: {:#x}",
            BootInfo::GetKernelVirtualAddress());
    Stacktrace::Initialize();

    Panic("Test Panic");
    for (;;) Arch::Halt();
}
