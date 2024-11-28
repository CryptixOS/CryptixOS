/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include "Common.hpp"

#include "ACPI/ACPI.hpp"
#include "API/Syscall.hpp"

#include "Arch/CPU.hpp"
#include "Arch/InterruptManager.hpp"

#include "Drivers/Serial.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Scheduler.hpp"
#include "Scheduler/Thread.hpp"

#include "Utility/ELF.hpp"
#include "Utility/ICxxAbi.hpp"
#include "Utility/Stacktrace.hpp"

#include "VFS/Initrd/Initrd.hpp"
#include "VFS/VFS.hpp"

static Process* kernelProcess;

void            userThread2()
{
    static const char* str = "Hey";
    for (;;)
        __asm__ volatile(
            "movq $0, %%rax\n"
            "movq $2, %%rdi\n"
            "movq %0, %%rsi\n"
            "movq %1, %%rdx\n"
            "syscall\n"
            :
            : "r"(str), "r"(3ull)
            : "rax", "rdx", "rsi", "rdi");
}

void kernelThread()
{
    Assert(VFS::MountRoot("tmpfs"));
    Initrd::Initialize();

    LogTrace("Loading user process...");
    Process* userProcess = new Process("TestUserProcess");
    userProcess->pageMap = VMM::GetKernelPageMap();
    userProcess->parent  = kernelProcess;

    ELF::Image program;
    uintptr_t  address = program.Load("/usr/sbin/init");
    Scheduler::EnqueueThread(new Thread(userProcess, address));
    Scheduler::EnqueueThread(
        new Thread(userProcess, reinterpret_cast<uintptr_t>(userThread2)));

    for (;;) Arch::Halt();
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

    kernelProcess          = new Process("Kernel Process");
    kernelProcess->pageMap = VMM::GetKernelPageMap();

    auto thread
        = new Thread(kernelProcess, reinterpret_cast<uintptr_t>(kernelThread),
                     0, CPU::GetCurrent()->id);

    Scheduler::EnqueueThread(thread);

    Syscall::InstallAll();
    Scheduler::Initialize();
    LogInfo("Kernel Stack Size: {:#x}, User Stack Size: {:#x}",
            CPU::KERNEL_STACK_SIZE, CPU::USER_STACK_SIZE);

    Scheduler::PrepareAP(true);

    for (;;) Arch::Halt();
}
