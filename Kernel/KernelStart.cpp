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

#include "Drivers/MemoryDevices.hpp"
#include "Drivers/Serial.hpp"
#include "Drivers/TTY.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Scheduler.hpp"
#include "Scheduler/Thread.hpp"

#include "Utility/ELF.hpp"
#include "Utility/ICxxAbi.hpp"
#include "Utility/Stacktrace.hpp"

#include "VFS/INode.hpp"
#include "VFS/Initrd/Initrd.hpp"
#include "VFS/VFS.hpp"

static Process* kernelProcess;

void            userThread2()
{
    static const char* str = "Hey";

    isize              fd  = -1;
    __asm__ volatile(
        "movq $1, %%rax\n"
        "movq %1, %%rdi\n"
        "syscall\n"
        "movq %%rax, %0"
        : "=r"(fd)
        : "r"("/usr/include/elf.h")
        : "rax", "rdi");

    static char buffer[1024];
    usize       bytesRead = 0;

    __asm__ volatile(
        "movq $2, %%rax\n"
        "movq %1, %%rdi\n"
        "movq %2, %%rsi\n"
        "movq $1024, %%rdx\n"
        "syscall\n"
        "movq %%rax, %0"
        : "=r"(bytesRead)
        : "r"(fd), "r"(buffer)
        : "rax", "rdi", "rsi");

    buffer[1023] = 0;
    for (;;)
        __asm__ volatile(
            "movq $0, %%rax\n"
            "movq $2, %%rdi\n"
            "movq %0, %%rsi\n"
            "movq %1, %%rdx\n"
            "syscall\n"
            :
            : "r"(buffer), "r"(1023ull)
            : "rax", "rdx", "rsi", "rdi");

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

void IterateDirectories(INode* node, int spaceCount = 0)
{
    char* spaces = new char[spaceCount + 1];
    memset(spaces, ' ', spaceCount);
    spaces[spaceCount] = 0;
    for (auto child : node->GetChildren())
    {
        LogInfo("{}-{}", spaces, child.second->GetName().data());
        if (child.second->mountGate)
        {
            IterateDirectories(child.second->mountGate, spaceCount + 4);
            continue;
        }
        if (child.second->GetType() == INodeType::eDirectory)
            IterateDirectories(child.second, spaceCount + 4);
    }
}

void kernelThread()
{
    Assert(VFS::MountRoot("tmpfs"));
    Initrd::Initialize();

    VFS::CreateNode(VFS::GetRootNode(), "/dev", S_IFDIR, INodeType::eDirectory);
    Assert(VFS::Mount(VFS::GetRootNode(), "", "/dev", "devtmpfs"));

    TTY::Initialize();
    MemoryDevices::Initialize();

    INode* devNode = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), "/dev"));
    Assert(devNode);

    char str[100];
    memset(str, 'A', 99);
    str[99] = 0;

    for (auto child : devNode->Reduce(true)->GetChildren())
    {
        LogTrace("/dev/{}", child.second->GetName());
        child.second->Write(str, 0, 12);
    }

    for (;;) Arch::Halt();

    LogTrace("Loading user process...");
    Process* userProcess = new Process("TestUserProcess");
    userProcess->pageMap = VMM::GetKernelPageMap();
    userProcess->parent  = kernelProcess;

    ELF::Image                 program;
    [[maybe_unused]] uintptr_t address = program.Load("/usr/sbin/init");
    // Scheduler::EnqueueThread(new Thread(userProcess, address));
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
