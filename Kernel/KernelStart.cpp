/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Syscall.hpp>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Tuple.hpp>
#include <Prism/Utility/PathView.hpp>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>
#include <Prism/Core/UUID.hpp>
#include <Prism/Network/IPv4Address.hpp>
#include <limine.h>
#include <Prism/Utility/Optional.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <API/Posix/linux/fb.h>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Prism/String/StringUtils.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/ByteStream.hpp>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Config.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>
#include <Prism/Core/Singleton.hpp>
#include <Prism/Core/Iterator.hpp>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.KernelStart;

import :Debug.Assertions;
import :Library.Logger;
import :Library.Stacktrace;
import :API.Syscall;
import :Arch.CPU;
import :Boot.BootInfo;
import :Boot.CommandLine;
import :Drivers.ACPI.Bus;
import :Drivers.Core.CharacterDevice;
import :Drivers.PCI.PCI;
import :Drivers.Serial;
import :Drivers.TTY.TTY;
import :Drivers.TTY.VirtualConsole;
import :Drivers.USB.USB;
import :Drivers.Video.FramebufferDevice;
import :Firmware.ACPI.ACPI;
import :Firmware.DMI.SMBIOS;
import :Firmware.DeviceTree.DeviceTree;
import :Library.ExecutableProgram;
import :Library.ICxxAbi;
import :Library.Image;
import :Library.Module;
import :Memory.MM;
import :Memory.PMM;
import :Memory.ScopedMapping;
import :Memory.VMM;
import :Memory.Allocator.KernelHeap;
import :Scheduler.Process;
import :Scheduler.Scheduler;
import :Scheduler.Thread;
import :System.System;
import :Time.Time;
import :VFS.Filesystem;
import :VFS.INode;
import :VFS.Initrd.Initrd;
import :VFS.MountPoint;
import :VFS.VFS;
import :Version;

namespace EFI
{
    bool Initialize(Pointer systemTable, const EfiMemoryMap& memoryMap);
};
bool        g_LogTmpFs = false;

// CTOS_DEFINE_PER_CPU_CACHE_HOT(i32, s_Preempt);

static bool loadInitProcess(Path initPath)
{
    Process* initProcess
        = Scheduler::CreateProcess(nullptr, initPath, s_RootCredentials);
    initProcess->PageMap = VMM::GetKernelPageMap();

    Vector<StringView> argv;
    argv.PushBack(initPath);
    Vector<StringView> envp;
    envp.PushBack("TERM=linux");

    static ExecutableProgram program;
    PageMap*                 pageMap = new PageMap();
    if (!program.Load(initPath, pageMap, initProcess->AddressSpace()))
        return false;

    initProcess->PageMap = pageMap;
    Logger::DisableSink(LOG_SINK_TERMINAL);
    auto initThread
        = initProcess->CreateThread(argv, envp, program, CPU::GetCurrent()->ID);
    VMM::UnmapKernelInitCode();

    Scheduler::EnqueueThread(initThread.Raw());
    return true;
}

static void kernelThread()
{
    VFS::Initialize();

    CharacterDevice::RegisterBaseMemoryDevices();
    PCI::Initialize();

#if CTOS_ACPI_DISABLE == 0
    if (CommandLine::GetBoolean("acpi.enable").ValueOr(true)
        && ACPI::IsAvailable())
    {
        ACPI::Enable();
        ACPI::LoadNameSpace();
        // ACPI::EnumerateDevices();
        ACPI::Bus::Initialize();
    }
    PCI::InitializeIrqRoutes();
#endif

    if (!FramebufferDevice::Initialize())
        LogError("kernel: Failed to initialize fbdev");
    TTY::Initialize();
    if (!USB::Initialize()) LogWarn("USB: Failed to initialize");

    if (!System::LoadBuiltinModules())
        LogWarn("Kernel: Could not find any builtin drivers");

    if (!System::LoadExternalModules())
        LogWarn("Kernel: Failed to load the external modules");

    if (!System::DispatchModules())
        LogError("Kernel: Failed to dispatch kernel modules");

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

    VirtualConsole::SetupFramebuffers(framebuffers);
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
    Arch::InstallExceptions();
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
    Logger::EnableSink(LOG_SINK_SERIAL);
    // DONT MOVE END
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    System::PrepareBootModules(info.KernelModules);

    Device::Initialize();
#if CTOS_DEVICE_TREE_DISABLE == 0
    // if (CommandLine::GetBoolean("dtb.enable").ValueOr(true))
    //     DeviceTree::Initialize(info.DeviceTreeBlob);
#endif

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
    auto process = Scheduler::KernelProcess();
    auto thread
        = process->CreateThread(kernelThread, false, CPU::Current()->ID);

    Scheduler::EnqueueThread(thread.Raw());
    Syscall::InstallAll();
    Scheduler::PrepareAP(true);

    AssertNotReached();
}
