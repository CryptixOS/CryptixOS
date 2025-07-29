/*
 * Created by v1tr10l7 on 30.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

/* signal mask to be sent at exit */
constexpr usize CSIGNAL              = 0x000000ff;
/* set if VM shared between processes */
constexpr usize CLONE_VM             = 0x00000100;
/* set if fs info shared between processes */
constexpr usize CLONE_FS             = 0x00000200;
/* set if open files shared between processes   */
constexpr usize CLONE_FILES          = 0x00000400;

/* set if signal handlers and blocked signals shared */
constexpr usize CLONE_SIGHAND        = 0x00000800;
/* set if a pidfd should be placed in parent */
constexpr usize CLONE_PIDFD          = 0x00001000;
/* set if we want to let tracing continue on the child too */
constexpr usize CLONE_PTRACE         = 0x00002000;
/* set if the parent wants the child to wake it up on mm_release */
constexpr usize CLONE_VFORK          = 0x00004000;
/* set if we want to have the same parent as the cloner */
constexpr usize CLONE_PARENT         = 0x00008000;
/* Same thread group? */
constexpr usize CLONE_THREAD         = 0x00010000;
/* New mount namespace group */
constexpr usize CLONE_NEWNS          = 0x00020000;
/* share system V SEM_UNDO semantics */
constexpr usize CLONE_SYSVSEM        = 0x00040000;
/* create a new TLS for the child */
constexpr usize CLONE_SETTLS         = 0x00080000;
/* set the TID in the parent */
constexpr usize CLONE_PARENT_SETTID  = 0x00100000;
/* clear the TID in the child */
constexpr usize CLONE_CHILD_CLEARTID = 0x00200000;
/* Unused, ignored */
constexpr usize CLONE_DETACHED       = 0x00400000;
/* set if the tracing process can't force CLONE_PTRACE on this clone */
constexpr usize CLONE_UNTRACED       = 0x00800000;
/* set the TID in the child */
constexpr usize CLONE_CHILD_SETTID   = 0x01000000;
/* New cgroup namespace */
constexpr usize CLONE_NEWCGROUP      = 0x02000000;
/* New utsname namespace */
constexpr usize CLONE_NEWUTS         = 0x04000000;
/* New ipc namespace */
constexpr usize CLONE_NEWIPC         = 0x08000000;
/* New user namespace */
constexpr usize CLONE_NEWUSER        = 0x10000000;
/* New pid namespace */
constexpr usize CLONE_NEWPID         = 0x20000000;
/* New network namespace */
constexpr usize CLONE_NEWNET         = 0x40000000;
/* Clone io context */
constexpr usize CLONE_IO             = 0x80000000;

/* Clear any signal handler and reset to SIG_DFL. */
constexpr usize CLONE_CLEAR_SIGHAND  = 0x100000000ULL;
/* Clone into a specific cgroup given the right permissions. */
constexpr usize CLONE_INTO_CGROUP    = 0x200000000ULL;
/*
 * cloning flags intersect with CSIGNAL so can be used with unshare and clone3
 * syscalls only:
 */
/* New time namespace */
constexpr usize CLONE_NEWTIME        = 0x00000080;

/**
 * struct clone_args - arguments for the clone3 syscall
 * @flags:        Flags for the new process as listed above.
 *                All flags are valid except for CSIGNAL and
 *                CLONE_DETACHED.
 * @pidfd:        If CLONE_PIDFD is set, a pidfd will be
 *                returned in this argument.
 * @child_tid:    If CLONE_CHILD_SETTID is set, the TID of the
 *                child process will be returned in the child's
 *                memory.
 * @parent_tid:   If CLONE_PARENT_SETTID is set, the TID of
 *                the child process will be returned in the
 *                parent's memory.
 * @exit_signal:  The exit_signal the parent process will be
 *                sent when the child exits.
 * @stack:        Specify the location of the stack for the
 *                child process.
 *                Note, @stack is expected to point to the
 *                lowest address. The stack direction will be
 *                determined by the kernel and set up
 *                appropriately based on @stack_size.
 * @stack_size:   The size of the stack for the child process.
 * @tls:          If CLONE_SETTLS is set, the tls descriptor
 *                is set to tls.
 * @set_tid:      Pointer to an array of type *pid_t. The size
 *                of the array is defined using @set_tid_size.
 *                This array is used to select PIDs/TIDs for
 *                newly created processes. The first element in
 *                this defines the PID in the most nested PID
 *                namespace. Each additional element in the array
 *                defines the PID in the parent PID namespace of
 *                the original PID namespace. If the array has
 *                less entries than the number of currently
 *                nested PID namespaces only the PIDs in the
 *                corresponding namespaces are set.
 * @set_tid_size: This defines the size of the array referenced
 *                in @set_tid. This cannot be larger than the
 *                kernel's limit of nested PID namespaces.
 * @cgroup:       If CLONE_INTO_CGROUP is specified set this to
 *                a file descriptor for the cgroup.
 *
 * The structure is versioned by size and thus extensible.
 * New struct members must go at the end of the struct and
 * must be properly 64bit aligned.
 */
struct clone_args
{
    u64 flags;
    u64 pidfd;
    u64 child_tid;
    u64 parent_tid;
    u64 exit_signal;
    u64 stack;
    u64 stack_size;
    u64 tls;
    u64 set_tid;
    u64 set_tid_size;
    u64 cgroup;
};

/* sizeof first published struct */
constexpr usize CLONE_ARGS_SIZE_VER0      = 64;
/* sizeof second published struct */
constexpr usize CLONE_ARGS_SIZE_VER1      = 80;
/* sizeof third published struct */
constexpr usize CLONE_ARGS_SIZE_VER2      = 88;

/*
 * Scheduling policies
 */
constexpr usize SCHED_NORMAL              = 0;
constexpr usize SCHED_FIFO                = 1;
constexpr usize SCHED_RR                  = 2;
constexpr usize SCHED_BATCH               = 3;
/* SCHED_ISO: reserved but not implemented yet */
constexpr usize SCHED_IDLE                = 5;
constexpr usize SCHED_DEADLINE            = 6;
constexpr usize SCHED_EXT                 = 7;

/* Can be ORed in to make sure the process is reverted back to SCHED_NORMAL on
 * fork */
constexpr usize SCHED_RESET_ON_FORK       = 0x40000000;

/*
 * For the sched_{set,get}attr() calls
 */
constexpr usize SCHED_FLAG_RESET_ON_FORK  = 0x01;
constexpr usize SCHED_FLAG_RECLAIM        = 0x02;
constexpr usize SCHED_FLAG_DL_OVERRUN     = 0x04;
constexpr usize SCHED_FLAG_KEEP_POLICY    = 0x08;
constexpr usize SCHED_FLAG_KEEP_PARAMS    = 0x10;
constexpr usize SCHED_FLAG_UTIL_CLAMP_MIN = 0x20;
constexpr usize SCHED_FLAG_UTIL_CLAMP_MAX = 0x40;

constexpr usize SCHED_FLAG_KEEP_ALL
    = (SCHED_FLAG_KEEP_POLICY | SCHED_FLAG_KEEP_PARAMS);

constexpr usize SCHED_FLAG_UTIL_CLAMP
    = (SCHED_FLAG_UTIL_CLAMP_MIN | SCHED_FLAG_UTIL_CLAMP_MAX);

constexpr usize SCHED_FLAG_ALL
    = (SCHED_FLAG_RESET_ON_FORK | SCHED_FLAG_RECLAIM | SCHED_FLAG_DL_OVERRUN
       | SCHED_FLAG_KEEP_ALL | SCHED_FLAG_UTIL_CLAMP);
