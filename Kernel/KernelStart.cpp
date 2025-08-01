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

#include <Drivers/Core/CharacterDevice.hpp>
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

#include <Memory/MM.hpp>
#include <Memory/PMM.hpp>
#include <Memory/ScopedMapping.hpp>
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

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/EchFs/EchFs.hpp>
#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Fat32Fs/Fat32Fs.hpp>
#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/TmpFs/TmpFs.hpp>

namespace EFI
{
    bool Initialize(Pointer systemTable, const EfiMemoryMap& memoryMap);
};
bool g_LogTmpFs = false;

static void eternal()
{
    for (;;)
    {
        LogTrace("Kernel thread");
        auto status = Time::NanoSleep(5'000'000'000zu);
        if (!status)
        {
            LogError("Sleeping failed");
            Process::Current()->Exit(-1);
        }

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
    Scheduler::EnqueueThread(newThread.Raw());
    Scheduler::EnqueueThread(userThread.Raw());

    return true;
}

static void kernelThread()
{
    registerFilesystems();
    Assert(VFS::MountRoot("tmpfs"));
    Initrd::Initialize();

    VFS::CreateDirectory("/dev", 0755 | S_IFDIR);
    Assert(VFS::Mount(nullptr, "", "/dev", "devfs"));

    Scheduler::InitializeProcFs();

    CharacterDevice::RegisterBaseMemoryDevices();
    Arch::ProbeDevices();
    PCI::Initialize();

#if CTOS_ACPI_DISABLE == 0
    if (CommandLine::GetBoolean("acpi.enable").ValueOr(true)
        && ACPI::IsAvailable())
    {
        ACPI::Enable();
        ACPI::LoadNameSpace();
        ACPI::EnumerateDevices();
    }
    PCI::InitializeIrqRoutes();
#endif

    if (!FramebufferDevice::Initialize())
        LogError("kernel: Failed to initialize fbdev");
    TTY::Initialize();
    if (!USB::Initialize()) LogWarn("USB: Failed to initialize");

    if (!System::LoadModules())
        LogWarn("ELF: Could not find any builtin drivers");

    auto moduleDirectory
        = VFS::ResolvePath(VFS::RootDirectoryEntry(), "/lib/modules/")
              .ValueOr(VFS::PathResolution{})
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
    MM::PrepareInitialHeap(memoryInfo);
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

    LogDebug("BootInfo: Lowest allocated address => {:#x}",
             info.LowestAllocatedAddress.Raw());
    LogDebug("BootInfo: Highest allocated address => {:#x}",
             info.HighestAllocatedAddress.Raw());

    MM::Initialize();
    Serial::Initialize();
    // DONT MOVE END
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    System::PrepareBootModules(info.KernelModules);

    Device::Initialize();
    DeviceTree::Initialize(info.DeviceTreeBlob);

#if CTOS_ACPI_DISABLE == 0
    if (CommandLine::GetBoolean("acpi.enable").ValueOr(true) && info.RSDP)
        ACPI::LoadTables(info.RSDP);
#endif

    Assert(System::LoadKernelSymbols(info.KernelExecutable));
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

    Scheduler::EnqueueThread(thread.Raw());

    Syscall::InstallAll();
    Scheduler::PrepareAP(true);

    AssertNotReached();
}
