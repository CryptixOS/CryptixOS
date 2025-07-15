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

#include <Boot/BootInfo.hpp>
#include <Boot/CommandLine.hpp>

#include <Drivers/MemoryDevices.hpp>
#include <Drivers/PCI/PCI.hpp>
#include <Drivers/Serial.hpp>
#include <Drivers/TTY.hpp>
#include <Drivers/Terminal.hpp>
#include <Drivers/USB/USB.hpp>
#include <Drivers/Video/FramebufferDevice.hpp>

#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/DMI/SMBIOS.hpp>
#include <Firmware/DeviceTree/DeviceTree.hpp>

#include <Library/ExecutableProgram.hpp>
#include <Library/ICxxAbi.hpp>
#include <Library/Image.hpp>
#include <Library/Module.hpp>
#include <Library/Stacktrace.hpp>

#include <Memory/MemoryManager.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Memory/Allocator/KernelHeap.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Delegate.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <System/System.hpp>
#include <Time/Time.hpp>

#include <VFS/Filesystem.hpp>
#include <VFS/INode.hpp>
#include <VFS/Initrd/Initrd.hpp>
#include <VFS/MountPoint.hpp>
#include <VFS/VFS.hpp>

namespace EFI
{
    bool Initialize(Pointer systemTable, const EfiMemoryMap& memoryMap);
};

static void eternal()
{
    for (;;)
    {
        auto status = Time::NanoSleep(1000 * 1000 * 5);
        if (!status)
        {
            LogError("Sleeping failed");
            Process::Current()->Exit(-1);
        }

        LogTrace("Kernel thread");
        Arch::Pause();
    }
}

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

    static ExecutableProgram program;
    PageMap*                 pageMap = new PageMap();
    if (!program.Load(initPath, pageMap, userProcess->AddressSpace()))
        return false;

    userProcess->PageMap = pageMap;
    Logger::DisableSink(LOG_SINK_TERMINAL);
    auto userThread
        = userProcess->CreateThread(argv, envp, program, CPU::GetCurrent()->ID);
    VMM::UnmapKernelInitCode();

    (void)eternal;
    auto colonel   = Scheduler::GetKernelProcess();
    auto newThread = colonel->CreateThread(reinterpret_cast<uintptr_t>(eternal),
                                           false, CPU::Current()->ID);

    // auto current   = Thread::Current();
    // current->SetState(ThreadState::eExited);

    Scheduler::EnqueueThread(newThread);
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
    if (CommandLine::GetBoolean("acpi.enable").ValueOr(true) && ACPI::IsAvailable())
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

    IgnoreUnused(
        VFS::Mount(nullptr, "/dev/nvme0n2p2"_sv, "/mnt/ext2"_sv, "ext2fs"_sv));
    IgnoreUnused(VFS::Mount(nullptr, "/dev/nvme0n2p1"_sv, "/mnt/fat32"_sv,
                            "fat32fs"_sv));
    IgnoreUnused(
        VFS::Mount(nullptr, "/dev/nvme0n2p3"_sv, "/mnt/echfs"_sv, "echfs"_sv));

    if (!USB::Initialize()) LogWarn("USB: Failed to initialize");

    if (!System::LoadModules())
        LogWarn("ELF: Could not find any builtin drivers");

    auto moduleDirectory
        = VFS::ResolvePath(VFS::GetRootDirectoryEntry().Raw(), "/lib/modules/")
              .value_or(VFS::PathResolution{})
              .Entry;
    if (moduleDirectory)
    {
        for (const auto& [name, child] : moduleDirectory->Children())
            System::LoadModule(child);
    }

    LogTrace("Loading init process...");
    auto initPath = CommandLine::GetString("init");
    if (!loadInitProcess(initPath.Empty() ? "/usr/sbin/init" : initPath))
        Panic(
            "Kernel: Failed to load the init process, try to specify it's path "
            "using 'init=/path/sbin/init' boot parameter");

    for (;;) Arch::Halt();
}

static void setupLogging(Span<Framebuffer, DynamicExtent> framebuffers)
{
#ifdef CTOS_TARGET_X86_64
    #define SERIAL_LOG_ENABLE_DEFAULT false
#else
    #define SERIAL_LOG_ENABLE_DEFAULT true
#endif

    Logger::EnableSink(LOG_SINK_E9);
    Logger::EnableSink(LOG_SINK_TERMINAL);

    bool enabledSinks[3] = {
        CommandLine::GetBoolean("log.e9").ValueOr(true),
        CommandLine::GetBoolean("log.serial")
            .ValueOr(SERIAL_LOG_ENABLE_DEFAULT),
        CommandLine::GetBoolean("log.terminal").ValueOr(true),
    };

    Terminal::SetupFramebuffers(framebuffers);
    for (usize i = 0; i < sizeof(enabledSinks) / sizeof(enabledSinks[0]); i++)
    {
        if (enabledSinks[i]) Logger::EnableSink(Bit(i));
        else Logger::DisableSink(Bit(i));
    }
}
extern "C" KERNEL_INIT_CODE __attribute__((no_sanitize("address"))) void
kernelStart(const BootInformation& info)
{
#define CTOS_GDB_ATTACHED 0
#if CTOS_GDB_ATTACHED == 1
    __asm__ volatile("1: jmp 1b");
#endif

    // DONT MOVE START
    // -------------------------------------------------------------
    // NOTE(v1tr10l7): It is important that these calls happen at the very
    // beginning of the kernel initialization, because we want to have a working
    // Heap and Serial logging be available ASAP, as every other subsystem
    // depends on this
    auto& memoryInfo = info.MemoryInformation;
    MemoryManager::PrepareInitialHeap(memoryInfo);
    InterruptManager::InstallExceptions();
    CommandLine::Initialize(info.KernelCommandLine);

    LogTrace("Kernel: Available framebuffers => {}", info.Framebuffers.Size());
    setupLogging(info.Framebuffers);

    LogInfo(
        "Boot: Kernel loaded with {}-{} -> firmware type: {}, boot time: {}s",
        info.BootloaderName, info.BootloaderVersion,
        ToString(info.FirmwareType), info.DateAtBoot);

    LogInfo("Boot: Kernel Physical Address: {:#p}",
            memoryInfo.KernelPhysicalBase);
    LogInfo("Boot: Kernel Virtual Address: {:#p}",
            memoryInfo.KernelVirtualBase);

    Assert(System::LoadKernelSymbols(info.KernelExecutable));
    MemoryManager::Initialize();
    Serial::Initialize();
    // DONT MOVE END
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    System::PrepareBootModules(info.KernelModules);

    Device::Initialize();
    DeviceTree::Initialize(info.DeviceTreeBlob);

    if (CommandLine::GetBoolean("acpi.enable").ValueOr(true))
        ACPI::LoadTables(info.RSDP);
    Arch::Initialize();

    System::InitializeNumaDomains();
    if (!EFI::Initialize(info.EfiSystemTable, memoryInfo.EfiMemoryMap))
        LogError("EFI: Failed to initialize efi runtime services...");
    DMI::SMBIOS::Initialize(info.SmBios32Phys, info.SmBios64Phys);

    Time::Initialize(info.DateAtBoot);
    Scheduler::Initialize();
    auto process = Scheduler::GetKernelProcess();
    auto thread
        = process->CreateThread(kernelThread, false, CPU::GetCurrent()->ID);

    Scheduler::EnqueueThread(thread);

    Syscall::InstallAll();
    Scheduler::PrepareAP(true);

    AssertNotReached();
}
