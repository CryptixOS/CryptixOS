/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/time.h>

constexpr usize WNOHANG    = 0x00000001;
constexpr usize WUNTRACED  = 0x00000002;
constexpr usize WSTOPPED   = WUNTRACED;
constexpr usize WEXITED    = 0x00000004;
constexpr usize WCONTINUED = 0x00000008;
constexpr usize WNOWAIT    = 0x01000000; /* Don't reap, just poll status.  */

struct rusage
{
    /* Total amount of user time used.  */
    struct timeval ru_utime;
    /* Total amount of system time used.  */
    struct timeval ru_stime;
    /* Maximum resident set size (in kilobytes).  */
    isize          ru_maxrss;
    /* Amount of sharing of text segment memory
       with other processes (kilobyte-seconds).  */
    isize          ru_ixrss;
    /* Amount of data segment memory used (kilobyte-seconds).  */
    isize          ru_idrss;
    /* Amount of stack memory used (kilobyte-seconds).  */
    isize          ru_isrss;
    /* Number of soft page faults (i.e. those serviced by reclaiming
       a page from the list of pages awaiting reallocation.  */
    isize          ru_minflt;
    /* Number of hard page faults (i.e. those that required I/O).  */
    isize          ru_majflt;
    /* Number of times a process was swapped out of physical memory.  */
    isize          ru_nswap;
    /* Number of input operations via the file system.  Note: This
       and `ru_oublock' do not include operations with the cache.  */
    isize          ru_inblock;
    /* Number of output operations via the file system.  */
    isize          ru_oublock;
    /* Number of IPC messages sent.  */
    isize          ru_msgsnd;
    /* Number of IPC messages received.  */
    isize          ru_msgrcv;
    /* Number of signals delivered.  */
    isize          ru_nsignals;
    /* Number of voluntary context switches, i.e. because the process
       gave up the process before it had to (usually to wait for some
       resource to be available).  */
    isize          ru_nvcsw;
    /* Number of involuntary context switches, i.e. a higher priority process
       became runnable or the current process used up its time slice.  */
    isize          ru_nivcsw;
};
