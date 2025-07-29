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

    constexpr usize          ARCH_SET_GS = 0x1001;
    constexpr usize          ARCH_SET_FS = 0x1002;
    constexpr usize          ARCH_GET_FS = 0x1003;
    constexpr usize          ARCH_GET_GS = 0x1004;
    static ErrorOr<upointer> ArchPrCtl(isize opcode, upointer addr)
    {
#ifdef CTOS_TARGET_X86_64
        auto                           thread = CPU::GetCurrentThread();
        CPU::UserMemoryProtectionGuard guard;
        switch (opcode)
        {
            case ARCH_SET_GS:
                thread->SetGsBase(addr);
                CPU::SetKernelGSBase(thread->GsBase());
                break;
            case ARCH_SET_FS:
                thread->SetFsBase(addr);
                CPU::SetFSBase(thread->FsBase());
                break;
            case ARCH_GET_FS:
                *reinterpret_cast<upointer*>(addr) = thread->FsBase();
                break;
            case ARCH_GET_GS:
                *reinterpret_cast<upointer*>(addr) = thread->GsBase();
                break;

            default: return_err(-1, EINVAL);
        }
#else
        CtosUnused(ARCH_SET_GS);
        CtosUnused(ARCH_SET_FS);
        CtosUnused(ARCH_GET_FS);
        CtosUnused(ARCH_GET_GS);
#endif

        return 0;
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
        RegisterSyscall(ID::eSigProcMask, API::Process::SigProcMask);
        RegisterSyscall(ID::eIoCtl, API::VFS::IoCtl);
        RegisterSyscall(ID::ePRead64, API::VFS::PRead);
        RegisterSyscall(ID::ePWrite64, API::VFS::PWrite);
        RegisterSyscall(ID::eAccess, API::VFS::Access);
        // RegisterSyscall(ID::ePipe, API::VFS::Pipe);
        RegisterSyscall(ID::eSchedYield, API::Process::SchedYield);
        RegisterSyscall(ID::eDup, API::VFS::Dup);
        RegisterSyscall(ID::eDup2, API::VFS::Dup2);
        RegisterSyscall(ID::eNanoSleep, API::Process::NanoSleep);
        RegisterSyscall(ID::ePid, API::Process::Pid);
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
        RegisterSyscall(ID::eSetResGid, API::Process::SetResGid);
        RegisterSyscall(ID::eGet_pGid, API::Process::GetPGid);
        RegisterSyscall(ID::eSid, API::Process::GetSid);
        RegisterSyscall(ID::eUTime, API::VFS::UTime);
        RegisterSyscall(ID::eStatFs, API::VFS::StatFs);
        RegisterSyscall(ID::eArchPrCtl, ArchPrCtl);
        RegisterSyscall(ID::eSetTimeOfDay, API::Time::SetTimeOfDay);
        RegisterSyscall(ID::eMount, API::VFS::Mount);
        RegisterSyscall(ID::eReboot, API::System::Reboot);
        // RegisterSyscall(ID::eGetTid, Process::SysGetTid);
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
        RegisterSyscall(ID::eRenameAt2, API::VFS::RenameAt2);
    }
    void Handle(Arguments& args)
    {
        CPU::OnSyscallEnter(args.Index == ToUnderlying(ID::ePanic)
                                ? CPU::GetCurrent()->LastSyscallID
                                : args.Index);
#define LOG_SYSCALLS false
        // #if LOG_SYSCALLS == true || true
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
        // #endif

        if (args.Index >= 512
            || (!s_Syscalls.Contains(static_cast<ID>(args.Index))))
        {
            args.ReturnValue = -1;
            errno            = ENOSYS;
            LogError(
                "Undefined syscall: {}\nparams: {{ arg[0]: {}, arg[1]: {}, "
                "arg[2]: {}, arg[3]: {}, arg[4]: {}, arg[5]: {}, }}",
                args.Index, args.Get<u64>(0), args.Get<u64>(1),
                args.Get<u64>(2), args.Get<u64>(3), args.Get<u64>(4),
                args.Get<u64>(5));

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

            if (ret) args.ReturnValue = ret.value();
            else if (static_cast<ID>(args.Index) != ID::eMMap)
            {
                if (g_LogSyscalls)
                {
                    auto syscallID   = static_cast<ID>(args.Index);
                    auto syscallName = StringUtils::ToString(syscallID);
                    syscallName.RemovePrefix(1);

                    SyscallError("Syscall: '{}' caused error", syscallName);
                }
                args.ReturnValue = -ipointer(ret.error());
            }
            return;
        }

        CPU::OnSyscallLeave();
    }
} // namespace Syscall
