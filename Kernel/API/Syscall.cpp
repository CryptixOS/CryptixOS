/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/MM.hpp>
#include <API/Process.hpp>
#include <API/System.hpp>
#include <API/Time.hpp>
#include <API/VFS.hpp>
#include <Arch/CPU.hpp>

#include <Prism/String/StringUtils.hpp>
#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

AtomicBool g_LogSyscalls = false;

namespace Syscall
{
    void                           Initialize();

    UnorderedMap<ID, WrapperBase*> s_Syscalls;
    StringView                     GetName(usize index)
    {
        auto id = static_cast<ID>(index);
        return StringUtils::ToString(id);
    }

    void InstallAll()
    {
        Initialize();

        RegisterSyscall(ID::eRead, API::VFS::Read);
        RegisterSyscall(ID::eWrite, API::VFS::Write);
        RegisterSyscall(ID::eOpen, API::VFS::Open);
        RegisterSyscall(ID::eClose, API::VFS::Close);
        RegisterSyscall(ID::eStat, API::VFS::Stat);
        RegisterSyscall(ID::eFStat, API::VFS::FStat);
        RegisterSyscall(ID::eLStat, API::VFS::LStat);
        RegisterSyscall(ID::eLSeek, API::VFS::LSeek);
        RegisterSyscall(ID::eMMap, API::MM::MMap);
        RegisterSyscall(ID::eMProtect, API::MM::MProtect);
        RegisterSyscall(ID::eMUnMap, API::MM::MUnMap);
        RegisterSyscall(ID::eSigAction, API::Process::SigAction);
        RegisterSyscall(ID::eSigProcMask, API::Process::SigProcMask);
        RegisterSyscall(ID::eSigReturn, API::Process::SigReturn);
        RegisterSyscall(ID::eIoCtl, API::VFS::IoCtl);
        RegisterSyscall(ID::ePRead64, API::VFS::PRead);
        RegisterSyscall(ID::ePWrite64, API::VFS::PWrite);
        RegisterSyscall(ID::eAccess, API::VFS::Access);
        // RegisterSyscall(ID::ePipe, API::VFS::Pipe);
        RegisterSyscall(ID::eSchedYield, API::Process::SchedYield);
        RegisterSyscall(ID::eDup, API::VFS::Dup);
        RegisterSyscall(ID::eDup2, API::VFS::Dup2);
        RegisterSyscall(ID::eNanoSleep, API::Time::NanoSleep);
        RegisterSyscall(ID::eGetITimer, API::Time::GetITimer);
        RegisterSyscall(ID::eSetITimer, API::Time::SetITimer);
        RegisterSyscall(ID::ePid, API::Process::Pid);
        RegisterSyscall(ID::eSocket, API::VFS::Socket);
        RegisterSyscall(ID::eBind, API::VFS::Bind);
        RegisterSyscall(ID::eClone, API::Process::Clone);
        RegisterSyscall(ID::eFork, API::Process::Fork);
        RegisterSyscall(ID::eExecve, API::Process::Execve);
        RegisterSyscall(ID::eExit, API::Process::Exit);
        RegisterSyscall(ID::eWait4, API::Process::Wait4);
        RegisterSyscall(ID::eKill, API::Process::Kill);
        RegisterSyscall(ID::eUname, API::System::Uname);
        RegisterSyscall(ID::eFCntl, API::VFS::FCntl);
        RegisterSyscall(ID::eTruncate, API::VFS::Truncate);
        RegisterSyscall(ID::eFTruncate, API::VFS::FTruncate);
        RegisterSyscall(ID::eGetCwd, API::VFS::GetCwd);
        RegisterSyscall(ID::eChDir, API::VFS::ChDir);
        RegisterSyscall(ID::eFChDir, API::VFS::FChDir);
        RegisterSyscall(ID::eRename, API::VFS::Rename);
        RegisterSyscall(ID::eMkDir, API::VFS::MkDir);
        RegisterSyscall(ID::eRmDir, API::VFS::RmDir);
        RegisterSyscall(ID::eCreat, API::VFS::Creat);
        RegisterSyscall(ID::eLink, API::VFS::Link);
        RegisterSyscall(ID::eUnlink, API::VFS::Unlink);
        RegisterSyscall(ID::eSymlink, API::VFS::Symlink);
        RegisterSyscall(ID::eReadLink, API::VFS::ReadLink);
        RegisterSyscall(ID::eChMod, API::VFS::ChMod);
        RegisterSyscall(ID::eFChMod, API::VFS::FChMod);
        RegisterSyscall(ID::eUmask, API::Process::Umask);
        RegisterSyscall(ID::eGetTimeOfDay, API::Time::GetTimeOfDay);
        RegisterSyscall(ID::eGetResourceLimit, API::System::GetResourceLimit);
        RegisterSyscall(ID::eGetResourceUsage, API::System::GetResourceUsage);
        RegisterSyscall(ID::eGetUid, API::Process::GetUid);
        RegisterSyscall(ID::eGetGid, API::Process::GetGid);
        RegisterSyscall(ID::eSetUid, API::Process::SetUid);
        RegisterSyscall(ID::eSetGid, API::Process::SetGid);
        RegisterSyscall(ID::eGet_eUid, API::Process::GetEUid);
        RegisterSyscall(ID::eGet_eGid, API::Process::GetEGid);
        RegisterSyscall(ID::eSet_pGid, API::Process::SetPGid);
        RegisterSyscall(ID::eGet_pPid, API::Process::GetPPid);
        RegisterSyscall(ID::eGetPgrp, API::Process::GetPGrp);
        RegisterSyscall(ID::eSetSid, API::Process::SetSid);
        RegisterSyscall(ID::eSetReUid, API::Process::SetReUid);
        RegisterSyscall(ID::eSetReGid, API::Process::SetReGid);
        RegisterSyscall(ID::eSetResUid, API::Process::SetResUid);
        RegisterSyscall(ID::eGetResUid, API::Process::GetResUid);
        RegisterSyscall(ID::eSetResGid, API::Process::SetResGid);
        RegisterSyscall(ID::eGetResGid, API::Process::GetResGid);
        RegisterSyscall(ID::eGet_pGid, API::Process::GetPGid);
        RegisterSyscall(ID::eSid, API::Process::GetSid);
        RegisterSyscall(ID::eSigSuspend, API::Process::SigSuspend);
        RegisterSyscall(ID::eSigAltStack, API::Process::SigAltStack);
        RegisterSyscall(ID::eUTime, API::VFS::UTime);
        RegisterSyscall(ID::eStatFs, API::VFS::StatFs);
        RegisterSyscall(ID::ePrCtl, API::System::PrCtl);
        RegisterSyscall(ID::eArchPrCtl, API::System::ArchPrCtl);
        RegisterSyscall(ID::eSetTimeOfDay, API::Time::SetTimeOfDay);
        RegisterSyscall(ID::eSync, API::VFS::SyncFilesystems);
        RegisterSyscall(ID::eMount, API::VFS::Mount);
        RegisterSyscall(ID::eReboot, API::System::Reboot);
        RegisterSyscall(ID::eInitModule, API::System::InitModule);
        RegisterSyscall(ID::eGetTid, API::Process::GetTid);
        RegisterSyscall(ID::eGetDents64, API::VFS::GetDEnts64);
        RegisterSyscall(ID::eClockGetTime, API::Time::ClockGetTime);
        RegisterSyscall(ID::ePanic, API::System::SysPanic);
        RegisterSyscall(ID::eOpenAt, API::VFS::OpenAt);
        RegisterSyscall(ID::eMkDirAt, API::VFS::MkDirAt);
        RegisterSyscall(ID::eMkNodAt, API::VFS::MkNodAt);
        RegisterSyscall(ID::eFStatAt, API::VFS::FStatAt);
        RegisterSyscall(ID::eUnlinkAt, API::VFS::UnlinkAt);
        RegisterSyscall(ID::eRenameAt, API::VFS::RenameAt);
        RegisterSyscall(ID::eLinkAt, API::VFS::LinkAt);
        RegisterSyscall(ID::eSymlinkAt, API::VFS::SymlinkAt);
        RegisterSyscall(ID::eReadLinkAt, API::VFS::ReadLinkAt);
        RegisterSyscall(ID::eFChModAt, API::VFS::FChModAt);
        RegisterSyscall(ID::ePSelect6, API::VFS::PSelect6);
        RegisterSyscall(ID::eUtimensAt, API::VFS::UtimensAt);
        RegisterSyscall(ID::eDup3, API::VFS::Dup3);
        RegisterSyscall(ID::ePipe2, API::VFS::Pipe2);
        RegisterSyscall(ID::eSyncFs, API::VFS::SyncFs);
        RegisterSyscall(ID::eRenameAt2, API::VFS::RenameAt2);
        RegisterSyscall(ID::eClone3, API::Process::Clone3);
        RegisterSyscall(ID::eFutexWake, API::Process::FutexWake);
        RegisterSyscall(ID::eFutexWait, API::Process::FutexWait);
        RegisterSyscall(ID::eDebugLog, API::System::DebugLog);
    }
    void Handle(Arguments& args)
    {
        CPU::OnSyscallEnter(args.Index == ToUnderlying(ID::ePanic)
                                ? CPU::GetCurrent()->LastSyscallID
                                : args.Index);
        auto thread = Thread::Current();
        thread->OnSyscallEnter();

#define LOG_SYSCALLS false
        // #if LOG_SYSCALLS == true
        static isize previousSyscall = -1;
        g_LogSyscalls                = false;

        if (static_cast<isize>(args.Index) != previousSyscall && g_LogSyscalls)
        {
            auto syscallID   = static_cast<ID>(args.Index);
            auto syscallName = StringUtils::ToString(syscallID);
            syscallName.RemovePrefix(1);

            LogTrace(
                "Syscall[{}]: '{}'\nparams: {{ arg[0]: {}, arg[1]: {}, "
                "arg[2]: {}, arg[3]: {}, arg[4]: {}, arg[5]: {}, }}",
                args.Index, syscallName, args.Get<u64>(0), args.Get<u64>(1),
                args.Get<u64>(2), args.Get<u64>(3), args.Get<u64>(4),
                args.Get<u64>(5));
        }

        previousSyscall = args.Index;

        if (!s_Syscalls.Contains(static_cast<ID>(args.Index)))
        {
            args.ReturnValue = -1;
            errno            = ENOSYS;
            LogError(
                "Undefined syscall: {}\nparams: {{ arg[0]: {}, arg[1]: {}, "
                "arg[2]: {}, arg[3]: {}, arg[4]: {}, arg[5]: {}, }}",
                args.Index, args.Get<u64>(0), args.Get<u64>(1),
                args.Get<u64>(2), args.Get<u64>(3), args.Get<u64>(4),
                args.Get<u64>(5));

            thread->OnSyscallLeave();
            CPU::OnSyscallLeave();
            return;
        }

        errno                  = no_error;
        Array<upointer, 6> arr = {args.Args[0], args.Args[1], args.Args[2],
                                  args.Args[3], args.Args[4], args.Args[5]};
#define SYSCALL_LOG_ERR   false
        // #if SYSCALL_LOG_ERR == true || true
#define SyscallError(...) LogError(__VA_ARGS__)
        // #else
        //     #define SyscallError(...)
        // #endif

        if (s_Syscalls.Contains(static_cast<ID>(args.Index)))
        {
            auto ret = s_Syscalls[static_cast<ID>(args.Index)]->Run(arr);

            if (ret) args.ReturnValue = ret.Value();
            else if (static_cast<ID>(args.Index) != ID::eMMap)
            {
                if (g_LogSyscalls)
                {
                    auto syscallID   = static_cast<ID>(args.Index);
                    auto syscallName = StringUtils::ToString(syscallID);
                    syscallName.RemovePrefix(1);

                    SyscallError("Syscall: '{}' caused error", syscallName);
                }
                args.ReturnValue = static_cast<usize>(ret.Error()) != MAP_FAILED
                                     ? -ipointer(ret.Error())
                                     : MAP_FAILED;
            }

            thread->OnSyscallLeave();
            CPU::OnSyscallLeave();
            return;
        }

        thread->OnSyscallLeave();
        CPU::OnSyscallLeave();
    }
} // namespace Syscall
