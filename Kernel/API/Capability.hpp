/*
 * Created by v1tr10l7 on 14.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/linux/capability.h>

enum class Capability : isize
{
    eNone               = -1,
    eChangeOwner        = CAP_CHOWN,
    eDacOverride        = CAP_DAC_OVERRIDE,
    eDacReadSearch      = CAP_DAC_READ_SEARCH,
    eOverrideFileOwner  = CAP_FOWNER,
    eSetID              = CAP_FSETID,
    eKill               = CAP_KILL,
    eSetGroupID         = CAP_SETGID,
    eSetUserID          = CAP_SETUID,
    eSetPCap            = CAP_SETPCAP,
    eLinuxImmutable     = CAP_LINUX_IMMUTABLE,
    eNetBindService     = CAP_NET_BIND_SERVICE,
    eNetBroadcast       = CAP_NET_BROADCAST,
    eNetAdmin           = CAP_NET_ADMIN,
    eNetRaw             = CAP_NET_RAW,
    eIpcLock            = CAP_IPC_LOCK,
    eIpcOwner           = CAP_IPC_OWNER,
    eSysModule          = CAP_SYS_MODULE,
    eSysRawIO           = CAP_SYS_RAWIO,
    eChangeRoot         = CAP_SYS_CHROOT,
    eSysProcessTrace    = CAP_SYS_PTRACE,
    eSysPAcct           = CAP_SYS_PACCT,
    eSysAdmin           = CAP_SYS_ADMIN,
    eSysBoot            = CAP_SYS_BOOT,
    eSysNice            = CAP_SYS_NICE,
    eSysResource        = CAP_SYS_RESOURCE,
    eSysTime            = CAP_SYS_TIME,
    eSysTTYConfig       = CAP_SYS_TTY_CONFIG,
    eMakeNode           = CAP_MKNOD,
    eLease              = CAP_LEASE,
    eAuditWrite         = CAP_AUDIT_WRITE,
    eAuditControl       = CAP_AUDIT_CONTROL,
    eSetFCap            = CAP_SETFCAP,
    eMacOverride        = CAP_MAC_OVERRIDE,
    eMacAdmin           = CAP_MAC_ADMIN,
    eSysLog             = CAP_SYSLOG,
    eWakeAlarm          = CAP_WAKE_ALARM,
    eBlockSuspend       = CAP_BLOCK_SUSPEND,
    eAuditRead          = CAP_AUDIT_READ,
    ePerformanceMonitor = CAP_PERFMON,
    eBPF                = CAP_BPF,
    eCheckpointRestore  = CAP_CHECKPOINT_RESTORE,
    eCount              = CAP_CHECKPOINT_RESTORE,
};

inline constexpr bool operator&(const Capability& lhs, Capability rhs)
{
    return ToUnderlying(lhs) & ToUnderlying(rhs);
}
