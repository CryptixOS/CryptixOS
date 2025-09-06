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
#include <Time/Time.hpp>

namespace API::Process
{
    using ::Process;

    ErrorOr<isize> SigAction(isize signal, const struct sigaction* action,
                             sigaction* oldAction)
    {
        auto process = Process::Current();
        auto thread  = Thread::Current();

        if (signal < 1 || signal > _NSIG
            || (action && (signal == SIGKILL || signal == SIGSTOP)))
            return Error(EINVAL);
        if ((action && !process->ValidateRead(action))
            || (oldAction && !process->ValidateWrite(oldAction)))
            return Error(EFAULT);

        if (oldAction)
        {
            const SignalAction& oldSignalAction
                = process->SignalAction(static_cast<SignalID>(signal));

            struct sigaction old{};
            old.sa_handler  = oldSignalAction.VirtualAddress;
            old.sa_flags    = oldSignalAction.Flags;
            old.sa_restorer = nullptr;
            *reinterpret_cast<usize*>(&old.sa_mask) = oldSignalAction.Mask;
            CPU::CopyToUser(oldAction, old);
        }

        if (!action) return 0;
        SignalAction newAction{};
        {
            CPU::UserMemoryProtectionGuard guard;
            newAction.VirtualAddress = action->sa_handler;
            newAction.Flags          = action->sa_flags;
            newAction.Mask = *reinterpret_cast<const u32*>(&action->sa_mask);
        }

        process->SetSignalAction(static_cast<SignalID>(signal), newAction);
        auto mask
            = thread->SignalMask() & ~(Bit(SIGKILL - 1) | Bit(SIGSTOP - 1));
        thread->SetSignalMask(mask);

        // if (sigact.sa_handler == SIG_IGN
        //     || (sigact.sa_handler == SIG_DFL
        //         && (signal == SIGCONT || signal == SIGCHLD || signal ==
        //         SIGURG
        //             || signal == SIGWINCH)))
        // TODO(v1tr10l7): remove signal from the queue, and recalc pending
        // signals
        ;

        return 0;
    }
    ErrorOr<isize> SigProcMask(i32 how, const sigset_t* set, sigset_t* oldSet)
    {
        auto process     = ::Process::Current();
        auto thread      = Thread::Current();
        auto currentMask = thread->SignalMask();
        return Error(ENOSYS);

        if (oldSet)
        {
            if (!process->ValidateWrite(oldSet, sizeof(sigset_t)))
                return Error(EFAULT);

            CPU::CopyToUser(reinterpret_cast<usize*>(oldSet), currentMask);
        }

        if (!set) return 0;
        if (!process->ValidateRead(set, sizeof(sigset_t))) return Error(EFAULT);

        auto newSet = CPU::CopyFromUser(*reinterpret_cast<const usize*>(set));
        switch (how)
        {
            case SIG_BLOCK: currentMask &= ~newSet; break;
            case SIG_UNBLOCK: currentMask |= newSet; break;
            case SIG_SETMASK: currentMask = newSet; break;

            default: return Error(EINVAL);
        }

        thread->SetSignalMask(currentMask);
        return 0;
    }
    ErrorOr<isize> SigReturn()
    {
        auto thread  = Thread::Current();
        auto success = thread->SignalReturn();
        if (!success) return Error(success.Error());

        Scheduler::Yield();
        return 0;
    }

    ErrorOr<isize> SchedYield()
    {
        Scheduler::Yield();
        return 0;
    }
    ErrorOr<isize> NanoSleep(const timespec* duration, timespec* rem)
    {
        auto current = Process::Current();
        if (!current->ValidateRead(duration, sizeof(timespec))
            || (rem && !current->ValidateRead(rem, sizeof(timespec))))
            return Error(EFAULT);

        auto time = CPU::CopyFromUser(*duration);
        auto r    = CPU::CopyFromUser(*rem);
        if (time.tv_sec < 0 || time.tv_nsec < 0
            || (rem && (r.tv_sec < 0 || r.tv_nsec < 0)))
            return Error(EINVAL);

        usize ns     = time.tv_nsec ?: time.tv_sec * 1'000'000'000;
        auto  status = Time::NanoSleep(ns);
        if (!status) return Error(status.Error());

        r.tv_sec = r.tv_nsec = 0;
        CPU::CopyToUser(rem, r);
        return 0;
    }

    ErrorOr<pid_t> Pid()
    {
        auto process = ::Process::Current();
        return process->Pid();
    }

    ErrorOr<pid_t> Fork()
    {
        class Process* process = ::Process::Current();
        Assert(process);

        CPU::SetInterruptFlag(false);
        auto newProcess = TryOrRet(process->Fork());
        Assert(newProcess);

        return newProcess->Pid();
    }
    ErrorOr<isize> Execve(char* pathname, char** argv, char** envp)
    {
        CPU::SetInterruptFlag(false);

        auto process = Process::Current();
        Path path    = CPU::CopyStringFromUser(pathname);

        process->Exec(path.Raw(), argv, envp);
        return process->Exit(-1);
    }
    ErrorOr<isize> Exit(isize exitcode)
    {
        auto* process = ::Process::Current();

        CPU::SetInterruptFlag(false);
        return process->Exit(exitcode);
    }
    ErrorOr<isize> Wait4(pid_t pid, isize* wstatus, isize flags, rusage* rusage)
    {
        auto           thread  = Thread::Current();
        class Process* process = thread->Parent();

        return process->WaitPid(pid, reinterpret_cast<i32*>(wstatus), flags,
                                rusage);
    }
    ErrorOr<isize> Kill(pid_t pid, isize signal)
    {
        class Process* current = ::Process::Current();
        class Process* target  = nullptr;

        if (pid == 0) target = current;
        else if (pid > 0) target = Scheduler::GetProcess(pid);
        else if (pid == -1)
        {
            ToDoWarn();
            // Send to everyone
        }
        else if (pid < -1)
        {
            // Send to everyone in group
            ToDoWarn();
            target = Scheduler::GetProcess(-pid);
        }

        LogTrace("API: Sending signal {} to process with pid => {}", signal,
                 pid);
        target->SendSignal(signal);
        return 0;
    }

    ErrorOr<mode_t> Umask(mode_t mask)
    {
        auto process = ::Process::Current();
        return process->Umask(mask);
    }
    ErrorOr<uid_t> GetUid()
    {
        auto process = ::Process::Current();
        return process->Credentials().UserID;
    }
    ErrorOr<gid_t> GetGid()
    {
        auto process = ::Process::Current();
        return process->Credentials().GroupID;
    }
    ErrorOr<isize> SetUid(uid_t uid)
    {
        LogDebug("API: SetUid => {}", uid);
        auto process = Process::Current();
        process->SetUID(uid);

        return {};
    }
    ErrorOr<isize> SetGid(gid_t gid)
    {
        LogDebug("API: SetGid => {}", gid);
        auto process = Process::Current();
        process->SetGID(gid);

        return {};
    }
    ErrorOr<uid_t> GetEUid()
    {
        auto process = ::Process::Current();
        return process->Credentials().EffectiveUserID;
    }
    ErrorOr<gid_t> GetEGid()
    {
        auto process = ::Process::Current();
        return process->Credentials().EffectiveGroupID;
    }
    ErrorOr<isize> SetPGid(pid_t pid, pid_t pgid)
    {
        class Process* current = ::Process::Current();
        class Process* process
            = pid ? Scheduler::GetProcess(pid) : ::Process::Current();

        if (!process) return Error(ESRCH);
        if ((process != current && !current->IsChild(process)))
            return Error(EPERM);

        if (pgid != 0)
        {
            if (pgid < 0) return Error(EINVAL);

            class Process* groupLeader = Scheduler::GetProcess(pgid);
            if (!groupLeader || process->Sid() != groupLeader->Sid())
                return Error(EPERM);

            process->SetPGid(pgid);
            return 0;
        }

        process->SetPGid(process->Pid());
        return 0;
    }

    ErrorOr<pid_t> GetPPid()
    {
        auto process = ::Process::Current();
        return process->ParentPid();
    }
    ErrorOr<pid_t> GetPGrp(pid_t pid) { return GetPGid(pid); }
    ErrorOr<pid_t> SetSid()
    {
        class Process* current = ::Process::Current();
        if (current->IsGroupLeader()) return Error(EPERM);

        return current->SetSid();
    }
    ErrorOr<isize> SetReUid(uid_t ruid, uid_t euid)
    {
        auto process = Process::Current();
        return process->SetReUID(ruid, euid);
    }
    ErrorOr<isize> SetReGid(gid_t rgid, gid_t egid)
    {
        auto process = Process::Current();
        return process->SetReGID(rgid, egid);
    }
    ErrorOr<isize> SetResUid(uid_t ruid, uid_t euid, uid_t suid)
    {
        auto process = Process::Current();
        return process->SetResUID(ruid, euid, suid);
    }
    ErrorOr<isize> SetResGid(gid_t rgid, gid_t egid, gid_t sgid)
    {
        auto process = Process::Current();
        return process->SetResGID(rgid, egid, sgid);
    }

    ErrorOr<pid_t> GetPGid(pid_t pid)
    {
        // FIXME(v1tr10l7): validate whether pid is a child of the calling
        // process
        class Process* currentProcess = ::Process::Current();

        if (pid == 0) return currentProcess->PGid();
        class Process* process = Scheduler::GetProcess(pid);
        if (!process
            || (process != currentProcess && !currentProcess->IsChild(process)))
            return Error(EPERM);

        return process->PGid();
    }
    ErrorOr<pid_t> GetSid(pid_t pid)
    {
        class Process* current = ::Process::Current();
        if (pid == 0) return current->Sid();

        class Process* process = Scheduler::GetProcess(pid);
        if (!process) return Error(ESRCH);

        if (current->Sid() != process->Sid()) return Error(EPERM);
        return process->Sid();
    }

    ErrorOr<usize> FutexWake(void* uaddr, usize mask, isize count, usize flags)
    {
        return Error(ENOSYS);
    }
    ErrorOr<usize> FutexWait(void* uaddr, usize value, usize mask, usize flags,
                             struct timespec* timeout, clockid_t clockid)
    {
        return Error(ENOSYS);
    }
} // namespace API::Process
