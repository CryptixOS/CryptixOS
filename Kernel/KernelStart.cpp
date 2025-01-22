/*
 * Created by v1tr10l7 on 16.11.2024.
 *
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <API/Syscall.hpp>

#include <Arch/CPU.hpp>
#include <Arch/InterruptManager.hpp>

#include <Drivers/MemoryDevices.hpp>
#include <Drivers/Serial.hpp>
#include <Drivers/TTY.hpp>

#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/SMBIOS/SMBIOS.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/ELF.hpp>
#include <Utility/ICxxAbi.hpp>
#include <Utility/Stacktrace.hpp>

#include <VFS/INode.hpp>
#include <VFS/Initrd/Initrd.hpp>
#include <VFS/VFS.hpp>

#include <magic_enum/magic_enum.hpp>

void kernelThread()
{
    Assert(VFS::MountRoot("tmpfs"));
    Initrd::Initialize();

    VFS::CreateNode(VFS::GetRootNode(), "/dev", 0755 | S_IFDIR);
    Assert(VFS::Mount(VFS::GetRootNode(), "", "/dev", "devtmpfs"));

    TTY::Initialize();
    MemoryDevices::Initialize();

    LogTrace("Loading user process...");
    Process* kernelProcess = Scheduler::GetKernelProcess();
    Process* userProcess   = Scheduler::CreateProcess(
        kernelProcess, "/usr/sbin/init", Credentials::s_Root);
    userProcess->PageMap = VMM::GetKernelPageMap();

    std::vector<std::string_view> argv;
    argv.push_back("/usr/sbin/init");
    std::vector<std::string_view> envp;
    envp.push_back("TERM=linux");

    static ELF::Image program, ld;
    PageMap*          pageMap = new PageMap();
    Assert(
        program.Load("/usr/sbin/init", pageMap, userProcess->m_AddressSpace));
    std::string_view ldPath = program.GetLdPath();
    if (!ldPath.empty())
    {
        LogTrace("Kernel: Loading ld: '{}'", ldPath);
        Assert(
            ld.Load(ldPath, pageMap, userProcess->m_AddressSpace, 0x40000000));
    }

    userProcess->PageMap = pageMap;
    uintptr_t address
        = ldPath.empty() ? program.GetEntryPoint() : ld.GetEntryPoint();
    Scheduler::EnqueueThread(new Thread(userProcess, address, argv, envp,
                                        program, CPU::GetCurrent()->ID));

    for (;;) Arch::Halt();
}

extern "C" __attribute__((no_sanitize("address"))) void kernelStart()
{
    InterruptManager::InstallExceptions();

    Assert(PMM::Initialize());
    icxxabi::Initialize();

    VMM::Initialize();
    Serial::Initialize();
    Logger::EnableOutput(LOG_OUTPUT_SERIAL);

    LogInfo(
        "Boot: Kernel loaded with {}-{} -> firmware type: {}, boot time: {}s",
        BootInfo::GetBootloaderName(), BootInfo::GetBootloaderVersion(),
        magic_enum::enum_name(BootInfo::GetFirmwareType()),
        BootInfo::GetBootTime());

    LogInfo("Boot: Kernel Physical Address: {:#x}",
            BootInfo::GetKernelPhysicalAddress().Raw<>());
    LogInfo("Boot: Kernel Virtual Address: {:#x}",
            BootInfo::GetKernelVirtualAddress().Raw<>());

    Stacktrace::Initialize();
    ACPI::Initialize();
    Arch::Initialize();

    SMBIOS::Initialize();

    auto thread = Scheduler::CreateKernelThread(
        reinterpret_cast<uintptr_t>(kernelThread), 0, CPU::GetCurrent()->ID);

    Scheduler::EnqueueThread(thread);

    Syscall::InstallAll();
    Scheduler::PrepareAP(true);

    std::unreachable();
}
