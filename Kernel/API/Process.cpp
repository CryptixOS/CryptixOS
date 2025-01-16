/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Process.hpp>

#include <Arch/InterruptGuard.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

namespace Syscall::Process
{
    uid_t SysGetUid(Syscall::Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetCredentials().uid;
    }
    gid_t SysGetGid(Syscall::Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetCredentials().gid;
    }

    uid_t SysGet_eUid(Syscall::Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetCredentials().euid;
    }
    gid_t SysGet_eGid(Syscall::Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetCredentials().egid;
    }

    pid_t SysGet_pPid(Syscall::Arguments&)
    {
        class Process* current = CPU::GetCurrentThread()->parent;
        Assert(current);

        return current->GetParentPid();
    }

    pid_t SysFork(Syscall::Arguments&)
    {
        class Process* process    = CPU::GetCurrentThread()->parent;

        class Process* newProcess = process->Fork();
        Assert(newProcess);

        LogInfo("New Process Pid: {}", newProcess->GetPid());
        return newProcess->GetPid();
    }
    int SysExecve(Syscall::Arguments& args)
    {
        char*  path = reinterpret_cast<char*>(args.Args[0]);
        char** argv = reinterpret_cast<char**>(args.Args[1]);
        char** envp = reinterpret_cast<char**>(args.Args[2]);

        return CPU::GetCurrentThread()->parent->Exec(path, argv, envp);
    }
    int SysExit(Syscall::Arguments& args)
    {
        i32   code    = args.Args[0];

        auto* process = CPU::GetCurrentThread()->parent;
        return process->Exit(code);
    }
}; // namespace Syscall::Process
