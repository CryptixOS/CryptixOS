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

        LogInfo("New Process Pid: {}", newProcess->GetPid());
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
        const pid_t pid  = static_cast<pid_t>(args.Args[0]);
        const pid_t pgid = static_cast<pid_t>(args.Args[1]);

        CtosUnused(pid);
        CtosUnused(pgid);

        // TODO(v1tr10l7): Set the process group id
        return 0;
    }

    ErrorOr<pid_t> SysGet_pPid(Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetParentPid();
    }
    ErrorOr<gid_t> SysGet_pGid(Syscall::Arguments& args)
    {
        // TODO(v1tr10l7): SysGet_pGid;

        return 0;
    }

}; // namespace Syscall::Process
