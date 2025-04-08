/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/sys/mman.h>
#include <API/Posix/sys/wait.h>
#include <API/Process.hpp>

#include <Arch/InterruptGuard.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

namespace API::Process
{
    ErrorOr<isize> SigProcMask(i32 how, const sigset_t* newSet,
                               sigset_t* oldSet)
    {
        auto     process     = ::Process::GetCurrent();
        Thread*  thread      = Thread::GetCurrent();
        sigset_t currentMask = thread->GetSignalMask();

        if (oldSet)
        {
            if (!process->ValidateWrite(oldSet, sizeof(sigset_t)))
                return Error(EFAULT);
            *oldSet = currentMask;
        }

        if (!newSet) return 0;
        if (!process->ValidateRead(newSet, sizeof(sigset_t)))
            return Error(EFAULT);

        switch (how)
        {
            case SIG_BLOCK: currentMask &= ~(*newSet); break;
            case SIG_UNBLOCK: currentMask |= *newSet; break;
            case SIG_SETMASK: currentMask = *newSet; break;

            default: return Error(EINVAL);
        }

        thread->SetSignalMask(currentMask);
        return 0;
    }
    ErrorOr<isize> SchedYield()
    {
        Scheduler::Yield();
        return 0;
    }

    ErrorOr<pid_t> GetPid()
    {
        auto process = ::Process::GetCurrent();
        return process->GetPid();
    }
    ErrorOr<mode_t> Umask(mode_t mask)
    {
        auto process = ::Process::GetCurrent();
        return process->Umask(mask);
    }
    ErrorOr<uid_t> GetUid()
    {
        auto process = ::Process::GetCurrent();
        return process->GetCredentials().uid;
    }
    ErrorOr<gid_t> GetGid()
    {
        auto process = ::Process::GetCurrent();
        return process->GetCredentials().gid;
    }
    ErrorOr<isize> SetUid(uid_t uid)
    {
        // auto process = ::Process::GetCurrent();
        return Error(ENOSYS);
    }
    ErrorOr<isize> SetGid(gid_t gid)
    {
        // auto process = ::Process::GetCurrent();
        return Error(ENOSYS);
    }
    ErrorOr<uid_t> GetEUid()
    {
        auto process = ::Process::GetCurrent();
        return process->GetCredentials().euid;
    }
    ErrorOr<gid_t> GetEGid()
    {
        auto process = ::Process::GetCurrent();
        return process->GetCredentials().egid;
    }
    ErrorOr<isize> SetPGid(pid_t pid, pid_t pgid);
    ErrorOr<pid_t> GetPPid()
    {
        auto process = ::Process::GetCurrent();
        return process->GetParentPid();
    }
    ErrorOr<pid_t> GetPGrp();
    ErrorOr<pid_t> SetSid();
    ErrorOr<pid_t> GetPGid();
    ErrorOr<pid_t> GetSid(pid_t pid);
} // namespace API::Process

namespace Syscall::Process
{
    ErrorOr<pid_t> SysFork(Arguments&)
    {
        class Process* process = ::Process::GetCurrent();

        CPU::SetInterruptFlag(false);
        auto procOrError = process->Fork();

        return procOrError ? procOrError.value()->GetPid()
                           : procOrError.error();
    }
    ErrorOr<i32> SysExecve(Arguments& args)
    {
        char*  path = reinterpret_cast<char*>(args.Args[0]);
        char** argv = reinterpret_cast<char**>(args.Args[1]);
        char** envp = reinterpret_cast<char**>(args.Args[2]);

        CPU::SetInterruptFlag(false);

        return ::Process::GetCurrent()->Exec(path, argv, envp);
    }
    ErrorOr<i32> SysExit(Arguments& args)
    {
        i32   code    = args.Args[0];

        auto* process = ::Process::GetCurrent();

        CPU::SetInterruptFlag(false);
        return process->Exit(code);
    }
    ErrorOr<i32> SysWait4(Arguments& args)
    {
        pid_t          pid     = static_cast<pid_t>(args.Args[0]);
        i32*           wstatus = reinterpret_cast<i32*>(args.Args[1]);
        i32            flags   = static_cast<i32>(args.Args[2]);
        rusage*        rusage  = reinterpret_cast<struct rusage*>(args.Args[3]);

        Thread*        thread  = CPU::GetCurrentThread();
        class Process* process = thread->GetParent();

        return process->WaitPid(pid, wstatus, flags, rusage);
    }
    ErrorOr<i32> SysKill(Arguments& args)
    {
        pid_t          pid     = args.Get<pid_t>(0);
        i32            sig     = args.Get<i32>(1);

        class Process* current = ::Process::GetCurrent();
        class Process* target  = nullptr;
        if (pid == 0) target = current;
        else if (pid > 0) target = Scheduler::GetProcess(pid);
        else if (pid == -1)
        {
            // Send to everyone
        }
        else if (pid < -1)
        {
            // Send to everyone in group
            target = Scheduler::GetProcess(-pid);
        }

        (void)target;
        (void)sig;

        return Error(ENOSYS);
    }

    ErrorOr<pid_t> SysSet_pGid(Syscall::Arguments& args)
    {
        const pid_t    pid     = static_cast<pid_t>(args.Args[0]);
        const pid_t    pgid    = static_cast<pid_t>(args.Args[1]);

        class Process* current = ::Process::GetCurrent();
        class Process* process
            = pid ? Scheduler::GetProcess(pid) : ::Process::GetCurrent();

        if (!process) return Error(ESRCH);
        if ((process != current && !current->IsChild(process)))
            return Error(EPERM);

        if (pgid != 0)
        {
            if (pgid < 0) return Error(EINVAL);

            class Process* groupLeader = Scheduler::GetProcess(pgid);
            if (!groupLeader || process->GetSid() != groupLeader->GetSid())
                return Error(EPERM);

            process->SetPGid(pgid);
            return 0;
        }

        process->SetPGid(process->GetPid());
        return 0;
    }

    ErrorOr<pid_t> SysGetPgrp(Syscall::Arguments& args)
    {
        args.Args[0] = 0;
        return SysGet_pGid(args);
    }
    ErrorOr<pid_t> SysSetSid(Arguments&)
    {
        class Process* current = ::Process::GetCurrent();
        if (current->IsGroupLeader()) return Error(EPERM);

        return current->SetSid();
    }
    ErrorOr<pid_t> SysGet_pGid(Arguments& args)
    {
        pid_t          pid            = args.Get<pid_t>(0);

        // FIXME(v1tr10l7): validate whether pid is a child of the calling
        // process
        class Process* currentProcess = ::Process::GetCurrent();

        if (pid == 0) return currentProcess->GetPGid();
        class Process* process = Scheduler::GetProcess(pid);
        if (!process
            || (process != currentProcess && !currentProcess->IsChild(process)))
            return Error(EPERM);

        return process->GetPGid();
    }
    ErrorOr<pid_t> SysGetSid(Arguments& args)
    {
        pid_t          pid     = args.Get<pid_t>(0);

        class Process* current = ::Process::GetCurrent();
        if (pid == 0) return current->GetSid();

        class Process* process = Scheduler::GetProcess(pid);
        if (!process) return Error(ESRCH);

        if (current->GetSid() != process->GetSid()) return Error(EPERM);
        return process->GetSid();
    }
    ErrorOr<i32> SysNanoSleep(Arguments& args)
    {
        const timespec* duration = args.Get<const timespec*>(0);
        timespec*       rem      = args.Get<timespec*>(1);

        (void)duration;
        (void)rem;
        // FIXME(v1tr10l7): sleep
        return Error(ENOSYS);
    }
}; // namespace Syscall::Process
