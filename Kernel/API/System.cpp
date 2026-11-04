/*
 * Created by v1tr10l7 on 15.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <format>
#include <API/Posix/asm/prctl.h>
#include <API/Posix/linux/prctl.h>
#include <API/Posix/sys/resource.h>
#include <API/Posix/sys/utsname.h>
#include <API/Syscall.hpp>
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
#include <API/UnixTypes.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
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
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <Prism/Core/Iterator.hpp>

module Cryptix:Impl.API.System;

import :API.System;
import :Arch.Arch;
import :Arch.PowerManager;
import :Drivers.TTY.VirtualConsole;
import :Scheduler.Process;
import :Scheduler.Thread;
import :System.System;
import :Version;

namespace API::System
{
    using namespace ::System;

    ErrorOr<isize> Uname(utsname* out)
    {
        auto process = Process::GetCurrent();
        if (!process->ValidateWrite(out)) return Error(EFAULT);

        UserMemoryProtectionGuard guard;
        Kernel::NAME.Copy(out->sysname, sizeof(out->sysname));
        Kernel::NAME.Copy(out->nodename, sizeof(out->nodename));
        Kernel::VERSION_STRING.Copy(out->release, sizeof(out->release));

        auto version = Kernel::BUILD_DATE + " ";
        version += Kernel::BUILD_TIME;
        version.Copy(out->version, sizeof(out->version));
        Kernel::ARCH_STRING.Copy(out->machine, sizeof(out->machine));

        return 0;
    }
    ErrorOr<isize> GetResourceLimit(isize resource, rlimit* rlimit)
    {
        // TODO(v1tr10l7): Set proper limits
        UserMemoryProtectionGuard guard;

        rlimit->rlim_cur = INT64_MAX;
        rlimit->rlim_max = INT64_MAX;
        return 0;

        switch (resource)
        {
            case RLIMIT_CPU: break;
            case RLIMIT_FSIZE: break;
            case RLIMIT_DATA: break;
            case RLIMIT_STACK: break;
            case RLIMIT_CORE:
                rlimit->rlim_cur = 64 * 0x1000;
                rlimit->rlim_max = 128 * 0x1000;
                break;

            case RLIMIT_RSS: break;
            case RLIMIT_NPROC: break;
            case RLIMIT_NOFILE: break;
            case RLIMIT_MEMLOCK: break;
            case RLIMIT_AS:
                rlimit->rlim_cur = INT32_MAX;
                rlimit->rlim_max = INT64_MAX;
                return 0;

            case RLIMIT_LOCKS: break;
            case RLIMIT_SIGPENDING: break;
            case RLIMIT_MSGQUEUE: break;
            case RLIMIT_NICE: break;

            case RLIMIT_RTPRIO: break;
            case RLIMIT_RTTIME: break;

            default:
                LogError("GetResourceLimit: Unknown resource id: {:#x}",
                         resource);
                return Error(EINVAL);
        }

        return Error(ENOSYS);
    }
    ErrorOr<isize> GetResourceUsage(isize who, rusage* usage)
    {
        return Error(ENOSYS);
    }

    ErrorOr<isize> PrCtl(isize opcode, upointer arg1, upointer arg2,
                         upointer arg3, upointer arg4)
    {
        LogDebug("API::System: PrCtl => opcode: {:#x}", opcode);
        return Error(ENOSYS);
    }
    ErrorOr<isize> ArchPrCtl(isize opcode, upointer arg1)
    {
#ifdef CTOS_TARGET_X86_64
        auto thread = Thread::Current();
        switch (opcode)
        {
            case ARCH_SET_GS:
                thread->SetGsBase(arg1);
                CPU::SetKernelGSBase(thread->GsBase());
                break;
            case ARCH_SET_FS:
                thread->SetFsBase(arg1);
                CPU::SetFSBase(thread->FsBase());
                break;
            case ARCH_GET_FS:
                CopyToUser(reinterpret_cast<upointer*>(arg1),
                           thread->FsBase().Raw());
                break;
            case ARCH_GET_GS:
                CopyToUser(reinterpret_cast<upointer*>(arg1),
                           thread->GsBase().Raw());
                break;

            default: return Error(EINVAL);
        }

        return 0;
#else
        return Error(ENOSYS);
#endif
    }

    ErrorOr<isize> Reboot(RebootCommand cmd)
    {
        switch (cmd)
        {
            case RebootCommand::eRestart: PowerManager::Reboot(); break;
            case RebootCommand::ePowerOff: PowerManager::PowerOff(); break;

            default: Prism::Log::Warn("SysReboot: Unknown OpCode!"); break;
        }

        return Error(ENOSYS);
    }
    ErrorOr<isize> InitModule(upointer moduleImage, usize imageSize,
                              const char** parameters)
    {
        // TODO(v1tr10l7): parameters
        auto process = Process::Current();
        if (!process->ValidateWrite(moduleImage, imageSize))
            return Error(EFAULT);
        if (!process->IsSuperUser()) return Error(EPERM);

        auto image = CreateRef<ELF::Image>();
        RetOnError(AsUser(
            [&image, moduleImage, imageSize]() -> auto
            {
                return image->LoadFromMemory(Pointer(moduleImage).As<u8>(),
                                             imageSize);
            }));

        auto status = System::LoadModule(image);
        if (!status) return Error(status.Error());

        return 0;
    }

    ErrorOr<upointer> SysPanic(const char* errorMessage)
    {
        UserMemoryProtectionGuard guard;

        EarlyLogMessage(AnsiColor::FOREGROUND_MAGENTA);
        EarlyLogMessage("SystemMemoryStatistics");
        EarlyLogMessage(AnsiColor::FOREGROUND_WHITE);
        EarlyLogMessage(": =>\n");

        EarlyLogMessage(AnsiColor::FOREGROUND_MAGENTA);
        EarlyLogMessage("TotalMemory");
        EarlyLogMessage(AnsiColor::FOREGROUND_WHITE);
        EarlyLogMessage(": %#p\n", PMM::GetTotalMemory());

        EarlyLogMessage(AnsiColor::FOREGROUND_MAGENTA);
        EarlyLogMessage("FreeMemory");
        EarlyLogMessage(AnsiColor::FOREGROUND_WHITE);
        EarlyLogMessage(": %#p\n", PMM::GetFreeMemory());

        EarlyLogMessage(AnsiColor::FOREGROUND_MAGENTA);
        EarlyLogMessage("UsedMemory");
        EarlyLogMessage(AnsiColor::FOREGROUND_WHITE);
        EarlyLogMessage(": %#p\n", PMM::GetUsedMemory());

        usize lastSyscall = CPU::Current()->LastSyscallID;
        auto  syscallName = Syscall::GetName(lastSyscall);
        panic(std::format("SYS_PANIC: {}\nLast called syscall: "
                          "{}\n",
                          errorMessage, syscallName)
                  .data());

        return -1;
    }
    ErrorOr<isize> DebugLog(const char* message)
    {
        auto string = CopyStringFromUser(message);
        LogDebug("{}", string);
        return 0;
    }
} // namespace API::System
