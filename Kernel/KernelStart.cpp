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

#include <Boot/CommandLine.hpp>

#include <Drivers/MemoryDevices.hpp>
#include <Drivers/PCI/PCI.hpp>
#include <Drivers/Serial.hpp>
#include <Drivers/TTY.hpp>

#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/DeviceTree/DeviceTree.hpp>
#include <Firmware/SMBIOS/SMBIOS.hpp>

#include <Library/ELF.hpp>
#include <Library/ICxxAbi.hpp>
#include <Library/Stacktrace.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Delegate.hpp>
#include <Prism/Endian.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

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
    VFS::CreateNode(VFS::GetRootNode(), "/mnt", 0755 | S_IFDIR);

    Scheduler::InitializeProcFs();

    PCI::Initialize();
    if (ACPI::IsAvailable())
    {
        ACPI::Enable();
        ACPI::LoadNameSpace();
        ACPI::EnumerateDevices();
    }
    PCI::InitializeIrqRoutes();

    TTY::Initialize();
    MemoryDevices::Initialize();

    Assert(VFS::Mount(VFS::GetRootNode(), "/dev/nvme0n2p1", "/mnt", "echfs"));

    auto kernelExecutable = BootInfo::GetExecutableFile();
    auto header   = reinterpret_cast<ELF::Header*>(kernelExecutable->address);
    auto sections = reinterpret_cast<ELF::SectionHeader*>(
        reinterpret_cast<u64>(kernelExecutable->address)
        + header->SectionHeaderTableOffset);
    auto stringTable = reinterpret_cast<char*>(
        reinterpret_cast<u64>(kernelExecutable->address)
        + sections[header->SectionNamesIndex].Offset);

    LogTrace("Loading kernel drivers");
    if (!ELF::Image::LoadModules(header->SectionEntryCount, sections,
                                 stringTable))
        LogError("ELF: Could not find any builtin drivers");

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

    Logger::DisableSink(LOG_SINK_TERMINAL);
    auto userThread = userProcess->CreateThread(address, argv, envp, program,
                                                CPU::GetCurrent()->ID);
    Scheduler::EnqueueThread(userThread);

    for (;;) Arch::Halt();
}

extern "C" __attribute__((no_sanitize("address"))) void kernelStart()
{
    InterruptManager::InstallExceptions();

    Assert(PMM::Initialize());
    icxxabi::Initialize();

    VMM::Initialize();
    Serial::Initialize();
    if (CommandLine::GetBoolean("log.serial").value_or(true))
        Logger::EnableSink(LOG_SINK_SERIAL);
    if (!CommandLine::GetBoolean("log.boot.terminal").value_or(true))
        Logger::DisableSink(LOG_SINK_SERIAL);
    if (!CommandLine::GetBoolean("log.e9").value_or(true))
        Logger::DisableSink(LOG_SINK_E9);

    LogInfo(
        "Boot: Kernel loaded with {}-{} -> firmware type: {}, boot time: {}s",
        BootInfo::GetBootloaderName(), BootInfo::GetBootloaderVersion(),
        magic_enum::enum_name(BootInfo::GetFirmwareType()),
        BootInfo::GetDateAtBoot());

    LogInfo("Boot: Kernel Physical Address: {:#x}",
            BootInfo::GetKernelPhysicalAddress().Raw<>());
    LogInfo("Boot: Kernel Virtual Address: {:#x}",
            BootInfo::GetKernelVirtualAddress().Raw<>());

    Stacktrace::Initialize();
    CommandLine::Initialize();
    DeviceTree::Initialize();

    ACPI::LoadTables();
    Arch::Initialize();

    SMBIOS::Initialize();

    Scheduler::Initialize();
    auto process = Scheduler::GetKernelProcess();
    auto thread
        = process->CreateThread(reinterpret_cast<uintptr_t>(kernelThread),
                                false, CPU::GetCurrent()->ID);

    Scheduler::EnqueueThread(thread);

    Syscall::InstallAll();
    Scheduler::PrepareAP(true);

    std::unreachable();
}
