/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/sys/wait.h>
#include <API/Process.hpp>

#include <Arch/InterruptGuard.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

namespace Syscall::Process
{
    ErrorOr<pid_t> SysGetPid(Arguments& args)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->m_Pid;
    }

    ErrorOr<pid_t> SysFork(Arguments&)
    {
        class Process* process = CPU::GetCurrentThread()->parent;

        CPU::SetInterruptFlag(false);
        class Process* newProcess = process->Fork();
        Assert(newProcess);

        return newProcess->GetPid();
    }
    ErrorOr<i32> SysExecve(Arguments& args)
    {
        char*  path = reinterpret_cast<char*>(args.Args[0]);
        char** argv = reinterpret_cast<char**>(args.Args[1]);
        char** envp = reinterpret_cast<char**>(args.Args[2]);

        CPU::SetInterruptFlag(false);

        return CPU::GetCurrentThread()->parent->Exec(path, argv, envp);
    }
    ErrorOr<i32> SysExit(Arguments& args)
    {
        i32   code    = args.Args[0];

        auto* process = CPU::GetCurrentThread()->parent;

        CPU::SetInterruptFlag(false);
        LogTrace("SysExit: exiting process: '{}'...", process->m_Name);
        return process->Exit(code);
    }
    ErrorOr<i32> SysWait4(Arguments& args)
    {
        pid_t          pid     = static_cast<pid_t>(args.Args[0]);
        i32*           wstatus = reinterpret_cast<i32*>(args.Args[1]);
        i32            flags   = static_cast<i32>(args.Args[2]);
        rusage*        rusage  = reinterpret_cast<struct rusage*>(args.Args[3]);

        Thread*        thread  = CPU::GetCurrentThread();
        class Process* process = thread->parent;

        return process->WaitPid(pid, wstatus, flags, rusage);
    }

    ErrorOr<uid_t> SysGetUid(Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetCredentials().uid;
    }
    ErrorOr<gid_t> SysGetGid(Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetCredentials().gid;
    }

    ErrorOr<uid_t> SysGet_eUid(Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetCredentials().euid;
    }
    ErrorOr<gid_t> SysGet_eGid(Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetCredentials().egid;
    }
    ErrorOr<gid_t> SysSet_pGid(Syscall::Arguments& args)
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

    ErrorOr<pid_t> SysGet_pPid(Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetParentPid();
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

}; // namespace Syscall::Process
