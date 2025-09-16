/*
 * Created by v1tr10l7 on 15.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Version.hpp>

#include <API/Posix/asm/prctl.h>
#include <API/Posix/linux/prctl.h>
#include <API/System.hpp>

#include <Arch/Arch.hpp>
#include <Arch/PowerManager.hpp>

#include <Drivers/Terminal.hpp>

#include <Prism/String/String.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>
#include <System/System.hpp>

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
