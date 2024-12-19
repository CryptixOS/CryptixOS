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
        char*  path = reinterpret_cast<char*>(args.args[0]);
        char** argv = reinterpret_cast<char**>(args.args[1]);
        char** envp = reinterpret_cast<char**>(args.args[2]);

        return CPU::GetCurrentThread()->parent->Exec(path, argv, envp);
    }
    int SysExit(Syscall::Arguments& args)
    {
        i32   code    = args.args[0];

        auto* process = CPU::GetCurrentThread()->parent;
        return process->Exit(code);
    }
}; // namespace Syscall::Process
