/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>
#include <Version.hpp>

#include <API/Syscall.hpp>

#include <Arch/CPU.hpp>
#include <Arch/InterruptManager.hpp>

#include <Boot/CommandLine.hpp>

#include <Drivers/FramebufferDevice.hpp>
#include <Drivers/MemoryDevices.hpp>
#include <Drivers/PCI/PCI.hpp>
#include <Drivers/Serial.hpp>
#include <Drivers/TTY.hpp>
#include <Drivers/Terminal.hpp>

#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/DeviceTree/DeviceTree.hpp>
#include <Firmware/SMBIOS/SMBIOS.hpp>

#include <Library/ELF.hpp>
#include <Library/ICxxAbi.hpp>
#include <Library/Image.hpp>
#include <Library/Module.hpp>
#include <Library/Stacktrace.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Delegate.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/String/StringView.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/INode.hpp>
#include <VFS/Initrd/Initrd.hpp>
#include <VFS/MountPoint.hpp>
#include <VFS/VFS.hpp>

#include <magic_enum/magic_enum.hpp>

namespace EFI
{
    bool Initialize();
};

static bool loadInitProcess(Path initPath)
{
    Process* kernelProcess = Scheduler::GetKernelProcess();
    Process* userProcess   = Scheduler::CreateProcess(kernelProcess, initPath,
                                                      Credentials::s_Root);
    userProcess->PageMap   = VMM::GetKernelPageMap();

    Vector<StringView> argv;
    argv.PushBack(initPath);
    Vector<StringView> envp;
    envp.PushBack("TERM=linux");

    static ELF::Image program, ld;
    PageMap*          pageMap = new PageMap();
    if (!program.Load(initPath, pageMap, userProcess->GetAddressSpace()))
        return false;
    PathView ldPath = program.GetLdPath();
    if (!ldPath.Empty()
        && !ld.Load(ldPath, pageMap, userProcess->GetAddressSpace(),
                    0x40000000))
    {
        delete pageMap;
        return false;
    }
    userProcess->PageMap = pageMap;
    auto address
        = ldPath.Empty() ? program.GetEntryPoint() : ld.GetEntryPoint();
    if (!address)
    {
        delete pageMap;
        return false;
    }

    Logger::DisableSink(LOG_SINK_TERMINAL);
    auto userThread = userProcess->CreateThread(address, argv, envp, program,
                                                CPU::GetCurrent()->ID);
    Scheduler::EnqueueThread(userThread);

    return true;
}

static void kernelThread()
{
    Assert(VFS::MountRoot("tmpfs"));
    Initrd::Initialize();

    VFS::CreateNode(nullptr, "/dev", 0755 | S_IFDIR);
    Assert(VFS::Mount(nullptr, "", "/dev", "devtmpfs"));

    Scheduler::InitializeProcFs();

    Arch::ProbeDevices();
    PCI::Initialize();
    if (ACPI::IsAvailable())
    {
        ACPI::Enable();
        ACPI::LoadNameSpace();
        ACPI::EnumerateDevices();
    }
    PCI::InitializeIrqRoutes();

    if (!FramebufferDevice::Initialize())
        LogError("kernel: Failed to initialize fbdev");
    TTY::Initialize();
    MemoryDevices::Initialize();

    VFS::Mount(nullptr, "/dev/nvme0n2p1"_sv, "/mnt/ext2"_sv, "ext2fs"_sv);
    VFS::Mount(nullptr, "/dev/nvme0n2p2"_sv, "/mnt/fat32"_sv, "fat32fs"_sv);
    VFS::Mount(nullptr, "/dev/nvme0n2p3"_sv, "/mnt/echfs"_sv, "echfs"_sv);

    auto    kernelExecutable = BootInfo::GetExecutableFile();
    Pointer imageAddress     = kernelExecutable->address;

    auto    header           = imageAddress.As<ELF::Header>();
    auto    sections
        = imageAddress.Offset<Pointer>(header->SectionHeaderTableOffset)
              .As<ELF::SectionHeader>();

    auto sectionNamesOffset = sections[header->SectionNamesIndex].Offset;
    auto stringTable
        = imageAddress.Offset<Pointer>(sectionNamesOffset).As<char>();

    ELF::Image kernelImage;
    kernelImage.LoadFromMemory(reinterpret_cast<u8*>(header),
                               kernelExecutable->size);

    LogTrace("Loading kernel drivers");
    if (!ELF::Image::LoadModules(header->SectionEntryCount, sections,
                                 stringTable))
        LogWarn("ELF: Could not find any builtin drivers");
    if (!Module::Load()) LogWarn("Module: Failed to find any modules");

    LogDebug("VFS: Testing directory entry caches...");
    auto pathRes = VFS::ResolvePath(VFS::GetRootDirectoryEntry(), "/mnt");
    auto entry   = pathRes.Entry->FollowMounts();
    for (auto& [name, dentry] : entry->Children())
    {
        LogTrace("Found directory => {}", name);
        LogTrace("Full path => {}", dentry->Path());
        LogTrace("Child entries =>");

        auto entry      = dentry;
        auto mountPoint = MountPoint::Lookup(entry);
        if (mountPoint) entry = mountPoint->HostEntry();
        entry->INode()->Populate();

        for (const auto& [childName, child] : entry->Children())
            LogTrace("\t- /{}", childName);
    }

    MountPoint* mountPoint = MountPoint::Head();

    LogTrace("Iterating mountpoints...");
    while (mountPoint)
    {
        LogTrace("{} => {}", mountPoint->HostEntry()->Name(),
                 mountPoint->Filesystem()->Name());
        mountPoint = mountPoint->NextMountPoint();
    }

    LogTrace("Loading init process...");
    auto initPath = CommandLine::GetString("init");
    if (!loadInitProcess(initPath.Empty() ? "/usr/sbin/init" : initPath))
        Panic(
            "Kernel: Failed to load the init process, try to specify it's path "
            "using 'init=/path/sbin/init' boot parameter");

    for (;;) Arch::Halt();
}

extern "C" KERNEL_INIT_CODE __attribute__((no_sanitize("address"))) void
kernelStart()
{
    InterruptManager::InstallExceptions();
#define CTOS_GDB_ATTACHED 0
#if CTOS_GDB_ATTACHED == 1
    __asm__ volatile("1: jmp 1b");
#endif

    Assert(PMM::Initialize());
    icxxabi::Initialize();

#ifdef CTOS_TARGET_X86_64
    #define SERIAL_LOG_ENABLE_DEFAULT false
#else
    #define SERIAL_LOG_ENABLE_DEFAULT true
#endif

    VMM::Initialize();
    Serial::Initialize();

    if (CommandLine::GetBoolean("log.serial")
            .value_or(SERIAL_LOG_ENABLE_DEFAULT))
        Logger::EnableSink(LOG_SINK_SERIAL);
    if (!CommandLine::GetBoolean("log.e9").value_or(true))
        Logger::DisableSink(LOG_SINK_E9);
    // if (CommandLine::GetBoolean("log.boot.terminal").value_or(true))
    Logger::EnableSink(LOG_SINK_TERMINAL);

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

    Device::Initialize();
    DeviceTree::Initialize();

    ACPI::LoadTables();
    Arch::Initialize();

    if (!EFI::Initialize())
        LogError("EFI: Failed to initialize efi runtime services...");
    SMBIOS::Initialize();

    Scheduler::Initialize();
    auto process = Scheduler::GetKernelProcess();
    auto thread
        = process->CreateThread(kernelThread, false, CPU::GetCurrent()->ID);

    Scheduler::EnqueueThread(thread);

    Syscall::InstallAll();
    Scheduler::PrepareAP(true);

    AssertNotReached();
}
