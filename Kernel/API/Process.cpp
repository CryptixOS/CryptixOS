/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/linux/ptrace.h>
#include <API/Posix/linux/sched.h>
#include <API/Posix/sys/mman.h>
#include <API/Posix/sys/wait.h>
#include <API/Process.hpp>
#include <Arch/InterruptGuard.hpp>

#include <Prism/String/StringUtils.hpp>
#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>
#include <Time/Time.hpp>

namespace API::Process
{
    using ::Process;
    static ErrorOr<Thread*> DoClone3(struct clone_args& args)
    {
        auto  current       = Process::Current();
        auto  currentThread = Thread::Current();

        usize flags         = args.flags;
        u64   newSp         = args.stack;

        if (flags & ~CLONE_VALID_FLAGS_MASK) return Error(EINVAL);
        if (flags & (CLONE_DETACHED | (CSIGNAL & ~CLONE_NEWTIME)))
            return Error(EINVAL);

        if ((flags & (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND))
            == (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND))
            return Error(EINVAL);
        if ((flags & (CLONE_THREAD | CLONE_PARENT)) && args.exit_signal)
            return Error(EINVAL);

        if (newSp)
        {
            if (!args.stack_size) return Error(EINVAL);
            if (!current->ValidateWrite(newSp, args.stack_size))
                return Error(EFAULT);
        }
        else if (args.stack_size > 0) return Error(EINVAL);

        if ((flags & CLONE_PIDFD) && (flags & CLONE_PARENT_SETTID)
            && (args.pidfd == args.parent_tid))
            return Error(EINVAL);

        isize trace = 0;
        if (!(flags & CLONE_UNTRACED))
        {
            if (flags & CLONE_VFORK) trace = PTRACE_EVENT_VFORK;
            else if (args.exit_signal != SIGCHLD) trace = PTRACE_EVENT_CLONE;
            else trace = PTRACE_EVENT_FORK;

            IgnoreUnused(trace);
        }

        Pointer tls    = args.tls;
        auto    cloned = TryOrRet(currentThread->Clone(
            flags & ~CLONE_PARENT_SETTID, newSp, args.stack_size, tls));
        Scheduler::EnqueueThread(cloned.Raw());
        return cloned.Raw();
    }

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
            CopyToUser(oldAction, old);
        }

        if (!action) return 0;
        SignalAction newAction{};
        {
            UserMemoryProtectionGuard guard;
            newAction.VirtualAddress = action->sa_handler;
            newAction.Flags          = action->sa_flags;
            newAction.Mask = *reinterpret_cast<const u32*>(&action->sa_mask);
        }

        process->SetSignalAction(static_cast<SignalID>(signal), newAction);
        auto mask = thread->SignalMask();
        mask.Remove(SIGKILL);
        mask.Remove(SIGSTOP);
        // & ~(Bit(SIGKILL - 1) | Bit(SIGSTOP - 1));

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
    ErrorOr<isize> SigProcMask(i32 how, const sigset_t* set, sigset_t* oldSet,
                               usize sigSetSize)
    {
        auto process = Process::Current();
        if (sigSetSize != sizeof(SignalSet)) return Error(EINVAL);

        auto thread      = Thread::Current();
        auto currentMask = thread->SignalMask();

        if (oldSet)
        {
            if (!process->ValidateWrite(oldSet, sizeof(SignalSet)))
                return Error(EFAULT);

            CopyToUser(reinterpret_cast<SignalSet*>(oldSet), currentMask);
        }

        if (!set) return 0;
        if (!process->ValidateRead(set, sizeof(SignalSet)))
            return Error(EFAULT);

        auto newSet = CopyFromUser(*reinterpret_cast<const SignalSet*>(set));
        newSet.Remove(SIGKILL);
        newSet.Remove(SIGSTOP);

        switch (how)
        {
            case SIG_BLOCK: currentMask |= newSet; break;
            case SIG_UNBLOCK: currentMask &= ~newSet; break;
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

    ErrorOr<ProcessID> Pid()
    {
        auto process = Process::Current();
        return process->ID();
    }

    ErrorOr<ProcessID> Clone(usize flags, usize newSp, i32* parentTid,
                             i32* childTid, usize tls)
    {
        auto            current      = Process::Current();

        constexpr usize stackSize    = CPU::USER_STACK_SIZE;
        u32             lowerFlags   = static_cast<u32>(flags);
        u64             outChildTid  = reinterpret_cast<u64>(childTid);
        u64             outParentTid = reinterpret_cast<u64>(parentTid);

        clone_args      args;
        Memory::Fill(&args, 0, sizeof(args));
        args.flags            = lowerFlags & ~CSIGNAL;
        args.pidfd            = outParentTid;
        args.child_tid        = outChildTid;
        args.parent_tid       = outParentTid;
        args.exit_signal      = lowerFlags & CSIGNAL;
        args.stack            = newSp;
        args.stack_size       = stackSize;
        args.tls              = tls;
        args.set_tid          = 0;
        args.set_tid_size     = 0;
        args.cgroup           = 0;

        auto     clonedThread = TryOrRet(DoClone3(args));
        ThreadID tid          = clonedThread->ID();

        if (flags & CLONE_CHILD_SETTID)
        {
            if (!childTid) return Error(EINVAL);
            if (!current->ValidateWrite(childTid, sizeof(i32)))
                return Error(EFAULT);

            CopyToUser(childTid, tid);
        }
        if (flags & CLONE_PARENT_SETTID)
        {
            if (!parentTid) return Error(EINVAL);
            if (!current->ValidateWrite(parentTid, sizeof(i32)))
                return Error(EFAULT);

            CopyToUser(parentTid, current->ID());
        }

        return clonedThread->Parent()->ID();
    }
    ErrorOr<ProcessID> Fork()
    {
        class Process* process = Process::Current();
        Assert(process);

        clone_args args   = {};
        args.flags        = 0 & ~CSIGNAL;
        // args.flags      = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND
        //            | CLONE_SYSVSEM | CLONE_SETTLS | CLONE_PARENT_SETTID;
        args.pidfd        = 0;
        args.child_tid    = 0;
        args.parent_tid   = 0;
        args.exit_signal  = SIGCHLD;
        args.stack        = 0;
        args.stack_size   = 0;
        args.tls          = 0;
        args.set_tid      = 0;
        args.set_tid_size = 0;
        args.cgroup       = 0;

        auto clonedThread = TryOrRet(DoClone3(args));
        auto newProcess   = clonedThread->Parent();
        return newProcess->ID();
    }
    ErrorOr<isize> Execve(char* pathname, char** argv, char** envp)
    {
        CPU::SetInterruptFlag(false);

        auto process = Process::Current();
        Path path    = CopyStringFromUser(pathname);

        process->Exec(path.Raw(), argv, envp);
        return process->Exit(-1);
    }
    ErrorOr<isize> Exit(isize exitcode)
    {
        auto process = Process::Current();

        CPU::SetInterruptFlag(false);
        return process->Exit(exitcode);
    }
    ErrorOr<isize> Wait4(ProcessID pid, isize* wstatus, isize flags,
                         rusage* rusage)
    {
        auto           thread  = Thread::Current();
        class Process* process = thread->Parent();

        return process->WaitPid(pid, reinterpret_cast<i32*>(wstatus), flags,
                                rusage);
    }
    ErrorOr<isize> Kill(ProcessID pid, isize signal)
    {
        auto current = Process::Current();
        LogDebug("API::Kill: Sending signal #{}({}) to process {}", signal,
                 StringUtils::ToString(static_cast<SignalID>(signal)), pid);

        if (signal < 1 || signal > _NSIG) return Error(EINVAL);
        if (pid > 0)
        {
            auto target = Scheduler::GetProcess(pid);
            if (!target) return Error(ESRCH);

            // TODO(v1tr10l7): perm check
            target->SendSignal(signal);
            return 0;
        }

        Vector<class Process*> targets;
        auto appendProcess = [&targets](auto process) -> IterationResult
        {
            targets.PushBack(process);
            return IterationResult::eContinue;
        };

        // Send to everyone you are permitted to, except 1
        if (pid == -1) Process::ForEach(appendProcess);
        else
        {
            GroupID targetGroup
                = pid == 0 ? current->Credentials().GroupID : -pid;
            Process::ForEachInGroup(targetGroup, appendProcess);
        }

        if (targets.Empty()) return Error(ESRCH);

        usize signalsDelivered = 0;
        for (auto target : targets)
        {
            // FIXME(v1tr10l7): perm check
            target->SendSignal(signal);
            ++signalsDelivered;
        }

        if (!signalsDelivered) return Error(EPERM);
        return 0;
    }

    ErrorOr<INodeMode> Umask(INodeMode mask)
    {
        auto process = Process::Current();
        return process->Umask(mask);
    }
    ErrorOr<UserID> GetUid()
    {
        auto process = Process::Current();
        return process->UserID();
    }
    ErrorOr<GroupID> GetGid()
    {
        auto process = Process::Current();
        return process->GroupID();
    }
    ErrorOr<isize> SetUid(UserID uid)
    {
        auto process = Process::Current();
        process->SetUserID(uid);

        return {};
    }
    ErrorOr<isize> SetGid(GroupID gid)
    {
        auto process = Process::Current();
        process->SetGroupID(gid);

        return {};
    }
    ErrorOr<UserID> GetEUid()
    {
        auto process = Process::Current();
        return process->EffectiveUserID();
    }
    ErrorOr<GroupID> GetEGid()
    {
        auto process = Process::Current();
        return process->EffectiveGroupID();
    }
    ErrorOr<isize> SetPGid(ProcessID pid, ProcessID pgid)
    {
        class Process* current = Process::Current();
        class Process* process
            = pid ? Scheduler::GetProcess(pid) : Process::Current();

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

        process->SetPGid(process->ID());
        return 0;
    }

    ErrorOr<ProcessID> GetPPid()
    {
        auto process = Process::Current();
        return process->ParentID();
    }
    ErrorOr<ProcessID> GetPGrp(ProcessID pid) { return GetPGid(pid); }
    ErrorOr<ProcessID> SetSid()
    {
        class Process* current = Process::Current();
        if (current->IsGroupLeader()) return Error(EPERM);

        return current->SetSid();
    }
    ErrorOr<isize> SetReUid(UserID ruid, UserID euid)
    {
        auto process = Process::Current();
        return process->SetReUID(ruid, euid);
    }
    ErrorOr<isize> SetReGid(GroupID rgid, GroupID egid)
    {
        auto process = Process::Current();
        return process->SetReGID(rgid, egid);
    }
    ErrorOr<isize> SetResUid(UserID ruid, UserID euid, UserID suid)
    {
        auto process = Process::Current();
        return process->SetResUID(ruid, euid, suid);
    }
    ErrorOr<isize> GetResUid(UserID* ruid, UserID* euid, UserID* suid)
    {
        auto process = Process::Current();
        CopyToUser(ruid, process->UserID());
        CopyToUser(euid, process->EffectiveUserID());
        CopyToUser(suid, process->Credentials().SetUserID);

        return 0;
    }
    ErrorOr<isize> SetResGid(GroupID rgid, GroupID egid, GroupID sgid)
    {
        auto process = Process::Current();
        return process->SetResGID(rgid, egid, sgid);
    }
    ErrorOr<isize> GetResGid(GroupID* rgid, GroupID* egid, GroupID* sgid)
    {
        auto process = Process::Current();
        CopyToUser(rgid, process->GroupID());
        CopyToUser(egid, process->EffectiveGroupID());
        CopyToUser(sgid, process->Credentials().SetGroupID);

        return 0;
    }

    ErrorOr<ProcessID> GetPGid(ProcessID pid)
    {
        // FIXME(v1tr10l7): validate whether pid is a child of the calling
        // process
        class Process* currentProcess = Process::Current();

        if (pid == 0) return currentProcess->PGid();
        class Process* process = Scheduler::GetProcess(pid);
        if (!process
            || (process != currentProcess && !currentProcess->IsChild(process)))
            return Error(EPERM);

        return process->PGid();
    }
    ErrorOr<ProcessID> GetSid(ProcessID pid)
    {
        class Process* current = Process::Current();
        if (pid == 0) return current->Sid();

        class Process* process = Scheduler::GetProcess(pid);
        if (!process) return Error(ESRCH);

        if (current->Sid() != process->Sid()) return Error(EPERM);
        return process->Sid();
    }

    ErrorOr<isize> SigAltStack(const struct sigaltstack* ss, sigaltstack* oldSs)
    {
        return Error(ENOSYS);
    }
    ErrorOr<ThreadID> GetTid()
    {
        auto thread = Thread::Current();

        return thread->ID();
    }

    ErrorOr<isize> Clone3(struct clone_args* uargs, usize size)
    {
        auto current = Process::Current();

        if (!uargs) return Error(EINVAL);
        if (!current->ValidateRead(uargs)) return Error(EFAULT);

        clone_args args         = CopyFromUser(*uargs, size);
        auto       flags        = args.flags;
        i32*       parentTid    = reinterpret_cast<i32*>(args.parent_tid);

        auto       clonedThread = TryOrRet(DoClone3(args));
        ThreadID   tid          = clonedThread->ID();

        if (flags & CLONE_PARENT_SETTID)
        {
            if (!parentTid) return Error(EINVAL);
            if (!current->ValidateWrite(parentTid, sizeof(i32)))
                return Error(EFAULT);

            CopyToUser(parentTid, tid);
        }

        return tid;
    }
    ErrorOr<usize> FutexWake(i32* uaddr, usize mask, isize count, usize flags)
    {
        auto process = Process::Current();

        auto status  = process->WakeFutex(uaddr);
        if (!status) return Error(status.Error());

        return 0;
    }
    ErrorOr<usize> FutexWait(i32* uaddr, usize value, usize mask, usize flags,
                             struct timespec* timeout, clockid_t clockid)
    {
        auto process = Process::Current();

        auto status  = process->WaitForFutex(uaddr, value);
        if (!status) return Error(status.Error());

        return 0;
    }
} // namespace API::Process
