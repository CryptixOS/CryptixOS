/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Syscall.hpp>

#include <API/MM.hpp>
#include <API/Process.hpp>
#include <API/System.hpp>
#include <API/Time.hpp>
#include <API/VFS.hpp>

#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <array>

namespace Syscall
{
    void Initialize();

    struct Syscall
    {
        std::string name;
        std::function<std::expected<uintptr_t, std::errno_t>(Arguments&)>
            handler;

        inline std::expected<uintptr_t, std::errno_t>
        operator()(Arguments& args)
        {
            if (handler.operator bool()) return handler(args);

            return 0;
        }
        inline operator bool() { return handler.operator bool(); }
    };
    static std::array<Syscall, 512> syscalls;

    void                            RegisterHandler(
                                   usize index,
                                   std::function<std::expected<uintptr_t, std::errno_t>(Arguments& args)>
                                               handler,
                                   std::string name)
    {
        syscalls[index] = {name, handler};
    }

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004
    static uintptr_t SysArchPrCtl(Arguments& args)
    {
        auto      thread = CPU::GetCurrentThread();
        i32       op     = args.Args[0];
        uintptr_t addr   = args.Args[1];

        switch (op)
        {
            case ARCH_SET_GS:
                thread->gsBase = addr;
                CPU::SetKernelGSBase(thread->gsBase);
                break;
            case ARCH_SET_FS:
                thread->fsBase = addr;
                CPU::SetFSBase(thread->fsBase);
                break;
            case ARCH_GET_FS:
                *reinterpret_cast<uintptr_t*>(addr) = thread->fsBase;
                break;
            case ARCH_GET_GS:
                *reinterpret_cast<uintptr_t*>(addr) = thread->gsBase;
                break;

            default: thread->error = EINVAL; return -1;
        }

        return 0;
    }

    void InstallAll()
    {
        auto sysPanic = [](Arguments& args) -> uintptr_t
        {
            const char* errorMessage
                = reinterpret_cast<const char*>(args.Args[0]);

            Panic("SYS_PANIC: {}", errorMessage);
        };

        Initialize();
        RegisterSyscall(ID::eRead, VFS::SysRead);
        RegisterSyscall(ID::eWrite, VFS::SysWrite);
        RegisterSyscall(ID::eOpen, VFS::SysOpen);
        RegisterSyscall(ID::eClose, VFS::SysClose);
        RegisterSyscall(ID::eStat, VFS::SysStat);
        RegisterSyscall(ID::eFStat, VFS::SysFStat);
        RegisterSyscall(ID::eLStat, VFS::SysLStat);
        RegisterSyscall(ID::eLSeek, VFS::SysLSeek);
        RegisterSyscall(ID::eMMap, MM::SysMMap);
        RegisterSyscall(ID::eIoCtl, VFS::SysIoCtl);
        RegisterSyscall(ID::eAccess, VFS::SysAccess);
        RegisterSyscall(ID::eDup, VFS::SysDup);
        RegisterSyscall(ID::eDup2, VFS::SysDup2);
        RegisterSyscall(ID::eGetPid, Process::SysGetPid);
        RegisterSyscall(ID::eExit, Process::SysExit);
        RegisterSyscall(ID::eWait4, Process::SysWait4);
        RegisterSyscall(ID::eGetUid, Process::SysGetUid);
        RegisterSyscall(ID::eGetGid, Process::SysGetGid);
        RegisterSyscall(ID::eUname, System::SysUname);
        RegisterSyscall(ID::eFcntl, VFS::SysFcntl);
        RegisterSyscall(ID::eGetCwd, VFS::SysGetCwd);
        RegisterSyscall(ID::eChDir, VFS::SysChDir);
        RegisterSyscall(ID::eFChDir, VFS::SysFChDir);
        RegisterSyscall(ID::eGetTimeOfDay, Time::SysGetTimeOfDay);
        RegisterSyscall(ID::eGet_eUid, Process::SysGet_eUid);
        RegisterSyscall(ID::eGet_eGid, Process::SysGet_eGid);
        RegisterSyscall(ID::eSet_pGid, Process::SysSet_pGid);
        RegisterSyscall(ID::eGet_pPid, Process::SysGet_pPid);
        RegisterSyscall(ID::eGet_pGid, Process::SysGet_pGid);
        RegisterSyscall(ID::eFork, Process::SysFork);
        RegisterSyscall(ID::eExecve, Process::SysExecve);
        RegisterSyscall(ID::eArchPrCtl, SysArchPrCtl);
        RegisterSyscall(ID::eSetTimeOfDay, Time::SysSetTimeOfDay);
        // RegisterSyscall(ID::eGetTid, Process::SysGetTid);
        RegisterSyscall(ID::eGetDents64, VFS::SysGetDents64);
        RegisterSyscall(ID::ePanic, sysPanic);
        RegisterSyscall(ID::eOpenAt, VFS::SysOpenAt);
        RegisterSyscall(ID::eFStatAt, VFS::SysFStatAt);
        RegisterSyscall(ID::eDup3, VFS::SysDup3);
    }
    void Handle(Arguments& args)
    {
#define LOG_SYSCALLS false
#if LOG_SYSCALLS == true
        static isize previousSyscall = -1;

        if (static_cast<isize>(args.Index) != previousSyscall)
            LogTrace("Syscall[{}]: '{}'", args.Index,
                     magic_enum::enum_name(static_cast<ID>(args.Index)));
        previousSyscall = args.Index;
#endif

        if (args.Index >= 512 || !syscalls[args.Index])
        {
            args.ReturnValue = -1;
            errno            = ENOSYS;
            LogError("Undefined syscall: {}", args.Index);
            return;
        }

        errno    = no_error;
        auto ret = syscalls[args.Index](args);

        if (ret) args.ReturnValue = ret.value();
        else args.ReturnValue = -intptr_t(ret.error());
    }
} // namespace Syscall
