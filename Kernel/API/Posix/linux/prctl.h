/*
 * Created by v1tr10l7 on 11.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

/* Values to pass as first argument to prctl() */
/* Second arg is a signal */
constexpr usize PR_SET_PDEATHSIG            = 1;
/* Second arg is a ptr to return the signal */
constexpr usize PR_GET_PDEATHSIG            = 2;

/* Get/set current->mm->dumpable */
constexpr usize PR_GET_DUMPABLE             = 3;
constexpr usize PR_SET_DUMPABLE             = 4;

/* Get/set unaligned access control bits (if meaningful) */
constexpr usize PR_GET_UNALIGN              = 5;
constexpr usize PR_SET_UNALIGN              = 6;
/* silently fix up unaligned user accesses */
constexpr usize PR_UNALIGN_NOPRINT          = 1;
/* generate SIGBUS on unaligned user access */
constexpr usize PR_UNALIGN_SIGBUS           = 2;

/* Get/set whether or not to drop capabilities on setuid() away from
 * uid 0 (as per security/commoncap.c) */
constexpr usize PR_GET_KEEPCAPS             = 7;
constexpr usize PR_SET_KEEPCAPS             = 8;

/* Get/set floating-point emulation control bits (if meaningful) */
constexpr usize PR_GET_FPEMU                = 9;
constexpr usize PR_SET_FPEMU                = 10;
/* silently emulate fp operations accesses */
constexpr usize PR_FPEMU_NOPRINT            = 1;
/* don't emulate fp operations, send SIGFPE instead */
constexpr usize PR_FPEMU_SIGFPE             = 2;

/* Get/set floating-point exception mode (if meaningful) */
constexpr usize PR_GET_FPEXC                = 11;
constexpr usize PR_SET_FPEXC                = 12;
/* Use FPEXC for FP exception enables */
constexpr usize PR_FP_EXC_SW_ENABLE         = 0x80;
/* floating point divide by zero */
constexpr usize PR_FP_EXC_DIV               = 0x010000;
/* floating point overflow */
constexpr usize PR_FP_EXC_OVF               = 0x020000;
/* floating point underflow */
constexpr usize PR_FP_EXC_UND               = 0x040000;
/* floating point inexact result */
constexpr usize PR_FP_EXC_RES               = 0x080000;
/* floating point invalid operation */
constexpr usize PR_FP_EXC_INV               = 0x100000;
/* FP exceptions disabled */
constexpr usize PR_FP_EXC_DISABLED          = 0;
/* async non-recoverable exc. mode */
constexpr usize PR_FP_EXC_NONRECOV          = 1;
/* async recoverable exception mode */
constexpr usize PR_FP_EXC_ASYNC             = 2;
/* precise exception mode */
constexpr usize PR_FP_EXC_PRECISE           = 3;

/* Get/set whether we use statistical process timing or accurate timestamp
 * based process timing */
constexpr usize PR_GET_TIMING               = 13;
constexpr usize PR_SET_TIMING               = 14;
constexpr usize PR_TIMING_STATISTICAL       = 0;
/* Normal, traditional, statistical process timing */
constexpr usize PR_TIMING_TIMESTAMP         = 1;
/* Accurate timestamp based process timing */

/* Set process name */
constexpr usize PR_SET_NAME                 = 15;
/* Get process name */
constexpr usize PR_GET_NAME                 = 16;

/* Get/set process endian */
constexpr usize PR_GET_ENDIAN               = 19;
constexpr usize PR_SET_ENDIAN               = 20;
constexpr usize PR_ENDIAN_BIG               = 0;
/* True little endian mode */
constexpr usize PR_ENDIAN_LITTLE            = 1;
/* "PowerPC" pseudo little endian */
constexpr usize PR_ENDIAN_PPC_LITTLE        = 2;

/* Get/set process seccomp mode */
constexpr usize PR_GET_SECCOMP              = 21;
constexpr usize PR_SET_SECCOMP              = 22;

/* Get/set the capability bounding set (as per security/commoncap.c) */
constexpr usize PR_CAPBSET_READ             = 23;
constexpr usize PR_CAPBSET_DROP             = 24;

/* Get/set the process' ability to use the timestamp counter instruction */
constexpr usize PR_GET_TSC                  = 25;
constexpr usize PR_SET_TSC                  = 26;
/* allow the use of the timestamp counter */
constexpr usize PR_TSC_ENABLE               = 1;
/* throw a SIGSEGV instead of reading the TSC */
constexpr usize PR_TSC_SIGSEGV              = 2;

/* Get/set securebits (as per security/commoncap.c) */
constexpr usize PR_GET_SECUREBITS           = 27;
constexpr usize PR_SET_SECUREBITS           = 28;

/*
 * Get/set the timerslack as used by poll/select/nanosleep
 * A value of 0 means "use default"
 */
constexpr usize PR_SET_TIMERSLACK           = 29;
constexpr usize PR_GET_TIMERSLACK           = 30;

constexpr usize PR_TASK_PERF_EVENTS_DISABLE = 31;
constexpr usize PR_TASK_PERF_EVENTS_ENABLE  = 32;

/*
 * Set early/late kill mode for hwpoison memory corruption.
 * This influences when the process gets killed on a memory corruption.
 */
constexpr usize PR_MCE_KILL                 = 33;
constexpr usize PR_MCE_KILL_CLEAR           = 0;
constexpr usize PR_MCE_KILL_SET             = 1;

constexpr usize PR_MCE_KILL_LATE            = 0;
constexpr usize PR_MCE_KILL_EARLY           = 1;
constexpr usize PR_MCE_KILL_DEFAULT         = 2;

constexpr usize PR_MCE_KILL_GET             = 34;

/*
 * Tune up process memory map specifics.
 */
constexpr usize PR_SET_MM                   = 35;
constexpr usize PR_SET_MM_START_CODE        = 1;
constexpr usize PR_SET_MM_END_CODE          = 2;
constexpr usize PR_SET_MM_START_DATA        = 3;
constexpr usize PR_SET_MM_END_DATA          = 4;
constexpr usize PR_SET_MM_START_STACK       = 5;
constexpr usize PR_SET_MM_START_BRK         = 6;
constexpr usize PR_SET_MM_BRK               = 7;
constexpr usize PR_SET_MM_ARG_START         = 8;
constexpr usize PR_SET_MM_ARG_END           = 9;
constexpr usize PR_SET_MM_ENV_START         = 10;
constexpr usize PR_SET_MM_ENV_END           = 11;
constexpr usize PR_SET_MM_AUXV              = 12;
constexpr usize PR_SET_MM_EXE_FILE          = 13;
constexpr usize PR_SET_MM_MAP               = 14;
constexpr usize PR_SET_MM_MAP_SIZE          = 15;

/*
 * This structure provides new memory descriptor
 * map which mostly modifies /proc/pid/stat[m]
 * output for a task. This mostly done in a
 * sake of checkpoint/restore functionality.
 */
struct prctl_mm_map
{
    /* code section bounds */
    u64  start_code;
    u64  end_code;
    /* data section bounds */
    u64  start_data;
    u64  end_data;
    /* heap for brk() syscall */
    u64  start_brk;
    u64  brk;
    /* stack starts at */
    u64  start_stack;
    /* command line arguments bounds */
    u64  arg_start;
    u64  arg_end;
    /* environment variables bounds */
    u64  env_start;
    u64  env_end;
    /* auxiliary vector */
    u64* auxv;
    /* vector size */
    u32  auxv_size;
    /* /proc/$pid/exe link file */
    u32  exe_fd;
};

/*
 * Set specific pid that is allowed to ptrace the current task.
 * A value of 0 mean "no process".
 */
constexpr usize PR_SET_PTRACER            = 0x59616d61;
constexpr usize PR_SET_PTRACER_ANY        = ((unsigned long)-1);

constexpr usize PR_SET_CHILD_SUBREAPER    = 36;
constexpr usize PR_GET_CHILD_SUBREAPER    = 37;

/*
 * If no_new_privs is set, then operations that grant new privileges (i.e.
 * execve) will either fail or not grant them.  This affects suid/sgid,
 * file capabilities, and LSMs.
 *
 * Operations that merely manipulate or drop existing privileges (setresuid,
 * capset, etc.) will still work.  Drop those privileges if you want them gone.
 *
 * Changing LSM security domain is considered a new privilege.  So, for example,
 * asking selinux for a specific new context (e.g. with runcon) will result
 * in execve returning -EPERM.
 *
 * See Documentation/userspace-api/no_new_privs.rst for more details.
 */
constexpr usize PR_SET_NO_NEW_PRIVS       = 38;
constexpr usize PR_GET_NO_NEW_PRIVS       = 39;

constexpr usize PR_GET_TID_ADDRESS        = 40;

constexpr usize PR_SET_THP_DISABLE        = 41;
constexpr usize PR_GET_THP_DISABLE        = 42;

/*
 * No longer implemented, but left here to ensure the numbers stay reserved:
 */
constexpr usize PR_MPX_ENABLE_MANAGEMENT  = 43;
constexpr usize PR_MPX_DISABLE_MANAGEMENT = 44;

constexpr usize PR_SET_FP_MODE            = 45;
constexpr usize PR_GET_FP_MODE            = 46;
/* 64b FP registers */
constexpr usize PR_FP_MODE_FR             = Bit(0);
/* 32b compatibility */
constexpr usize PR_FP_MODE_FRE            = Bit(1);

/* Control the ambient capability set */
constexpr usize PR_CAP_AMBIENT            = 47;
constexpr usize PR_CAP_AMBIENT_IS_SET     = 1;
constexpr usize PR_CAP_AMBIENT_RAISE      = 2;
constexpr usize PR_CAP_AMBIENT_LOWER      = 3;
constexpr usize PR_CAP_AMBIENT_CLEAR_ALL  = 4;

/* arm64 Scalable Vector Extension controls */
/* Flag values must be kept in sync with ptrace NT_ARM_SVE interface */
/* set task vector length */
constexpr usize PR_SVE_SET_VL             = 50;
/* defer effect until exec */
constexpr usize PR_SVE_SET_VL_ONEXEC      = Bit(18);
/* get task vector length */
constexpr usize PR_SVE_GET_VL             = 51;
/* Bits common to PR_SVE_SET_VL and PR_SVE_GET_VL */
constexpr usize PR_SVE_VL_LEN_MASK        = 0xffff;
/* inherit across exec */
constexpr usize PR_SVE_VL_INHERIT         = Bit(17);

/* Per task speculation control */
constexpr usize PR_GET_SPECULATION_CTRL   = 52;
constexpr usize PR_SET_SPECULATION_CTRL   = 53;
/* Speculation control variants */
constexpr usize PR_SPEC_STORE_BYPASS      = 0;
constexpr usize PR_SPEC_INDIRECT_BRANCH   = 1;
constexpr usize PR_SPEC_L1D_FLUSH         = 2;
/* Return and control values for PR_SET/GET_SPECULATION_CTRL */
constexpr usize PR_SPEC_NOT_AFFECTED      = 0;
constexpr usize PR_SPEC_PRCTL             = Bit(0);
constexpr usize PR_SPEC_ENABLE            = Bit(1);
constexpr usize PR_SPEC_DISABLE           = Bit(2);
constexpr usize PR_SPEC_FORCE_DISABLE     = Bit(3);
constexpr usize PR_SPEC_DISABLE_NOEXEC    = Bit(4);

/* Reset arm64 pointer authentication keys */
constexpr usize PR_PAC_RESET_KEYS         = 54;
constexpr usize PR_PAC_APIAKEY            = Bit(0);
constexpr usize PR_PAC_APIBKEY            = Bit(1);
constexpr usize PR_PAC_APDAKEY            = Bit(2);
constexpr usize PR_PAC_APDBKEY            = Bit(3);
constexpr usize PR_PAC_APGAKEY            = Bit(4);

/* Tagged user address controls for arm64 and RISC-V */
constexpr usize PR_SET_TAGGED_ADDR_CTRL   = 55;
constexpr usize PR_GET_TAGGED_ADDR_CTRL   = 56;
constexpr usize PR_TAGGED_ADDR_ENABLE     = Bit(0);
/* MTE tag check fault modes */
constexpr usize PR_MTE_TCF_NONE           = 0;
constexpr usize PR_MTE_TCF_SYNC           = Bit(1);
constexpr usize PR_MTE_TCF_ASYNC          = Bit(2);
/* MTE tag inclusion mask */
constexpr usize PR_MTE_TCF_MASK   = (PR_MTE_TCF_SYNC | PR_MTE_TCF_ASYNC);

constexpr usize PR_MTE_TAG_SHIFT  = 3;
constexpr usize PR_MTE_TAG_MASK   = (0xffffUL << PR_MTE_TAG_SHIFT);
/* Unused; kept only for source compatibility */
constexpr usize PR_MTE_TCF_SHIFT  = 1;
/* RISC-V pointer masking tag length */
constexpr usize PR_PMLEN_SHIFT    = 24;
constexpr usize PR_PMLEN_MASK     = (0x7fUL << PR_PMLEN_SHIFT);

/* Control reclaim behavior when allocating memory */
constexpr usize PR_SET_IO_FLUSHER = 57;
constexpr usize PR_GET_IO_FLUSHER = 58;

/* Dispatch syscalls to a userspace handler */
constexpr usize PR_SET_SYSCALL_USER_DISPATCH  = 59;
constexpr usize PR_SYS_DISPATCH_OFF           = 0;
constexpr usize PR_SYS_DISPATCH_ON            = 1;
/* The control values for the user space selector when dispatch is enabled */
constexpr usize SYSCALL_DISPATCH_FILTER_ALLOW = 0;
constexpr usize SYSCALL_DISPATCH_FILTER_BLOCK = 1;

/* Set/get enabled arm64 pointer authentication keys */
constexpr usize PR_PAC_SET_ENABLED_KEYS       = 60;
constexpr usize PR_PAC_GET_ENABLED_KEYS       = 61;

/* Request the scheduler to share a core */
constexpr usize PR_SCHED_CORE                 = 62;
constexpr usize PR_SCHED_CORE_GET             = 0;
/* create unique core_sched cookie */
constexpr usize PR_SCHED_CORE_CREATE          = 1;
/* push core_sched cookie to pid */
constexpr usize PR_SCHED_CORE_SHARE_TO        = 2;
constexpr usize PR_SCHED_CORE_SHARE_FROM
    = 3; /* pull core_sched cookie to pid */
constexpr usize PR_SCHED_CORE_MAX                 = 4;
constexpr usize PR_SCHED_CORE_SCOPE_THREAD        = 0;
constexpr usize PR_SCHED_CORE_SCOPE_THREAD_GROUP  = 1;
constexpr usize PR_SCHED_CORE_SCOPE_PROCESS_GROUP = 2;

/* arm64 Scalable Matrix Extension controls */
/* Flag values must be in sync with SVE versions */
/* set task vector length */
constexpr usize PR_SME_SET_VL                     = 63;
/* defer effect until exec */
constexpr usize PR_SME_SET_VL_ONEXEC              = Bit(18);
/* get task vector length */
constexpr usize PR_SME_GET_VL                     = 64;
/* Bits common to PR_SME_SET_VL and PR_SME_GET_VL */
constexpr usize PR_SME_VL_LEN_MASK                = 0xffff;
/* inherit across exec */
constexpr usize PR_SME_VL_INHERIT                 = Bit(17);

/* Memory deny write / execute */
constexpr usize PR_SET_MDWE                       = 65;
constexpr usize PR_MDWE_REFUSE_EXEC_GAIN          = Bit(0);
constexpr usize PR_MDWE_NO_INHERIT                = Bit(1);

constexpr usize PR_GET_MDWE                       = 66;

constexpr usize PR_SET_VMA                        = 0x53564d41;
constexpr usize PR_SET_VMA_ANON_NAME              = 0;

constexpr usize PR_GET_AUXV                       = 0x41555856;

constexpr usize PR_SET_MEMORY_MERGE               = 67;
constexpr usize PR_GET_MEMORY_MERGE               = 68;

constexpr usize PR_RISCV_V_SET_CONTROL            = 69;
constexpr usize PR_RISCV_V_GET_CONTROL            = 70;
constexpr usize PR_RISCV_V_VSTATE_CTRL_DEFAULT    = 0;
constexpr usize PR_RISCV_V_VSTATE_CTRL_OFF        = 1;
constexpr usize PR_RISCV_V_VSTATE_CTRL_ON         = 2;
constexpr usize PR_RISCV_V_VSTATE_CTRL_INHERIT    = Bit(4);
constexpr usize PR_RISCV_V_VSTATE_CTRL_CUR_MASK   = 0x3;
constexpr usize PR_RISCV_V_VSTATE_CTRL_NEXT_MASK  = 0xc;
constexpr usize PR_RISCV_V_VSTATE_CTRL_MASK       = 0x1f;

constexpr usize PR_RISCV_SET_ICACHE_FLUSH_CTX     = 71;
constexpr usize PR_RISCV_CTX_SW_FENCEI_ON         = 0;
constexpr usize PR_RISCV_CTX_SW_FENCEI_OFF        = 1;
constexpr usize PR_RISCV_SCOPE_PER_PROCESS        = 0;
constexpr usize PR_RISCV_SCOPE_PER_THREAD         = 1;

/* PowerPC Dynamic Execution Control Register (DEXCR) controls */
constexpr usize PR_PPC_GET_DEXCR                  = 72;
constexpr usize PR_PPC_SET_DEXCR                  = 73;
/* DEXCR aspect to act on */
/* Speculative branch hint enable */
constexpr usize PR_PPC_DEXCR_SBHE                 = 0;
/* Indirect branch recurrent target prediction disable */
constexpr usize PR_PPC_DEXCR_IBRTPD               = 1;
/* Subroutine return address prediction disable */
constexpr usize PR_PPC_DEXCR_SRAPD                = 2;
/* Non-privileged hash instruction enable */
constexpr usize PR_PPC_DEXCR_NPHIE                = 3;
/* Action to apply / return */
/* Aspect can be modified with PR_PPC_SET_DEXCR */
constexpr usize PR_PPC_DEXCR_CTRL_EDITABLE        = 0x1;
/* Set the aspect for this process */
constexpr usize PR_PPC_DEXCR_CTRL_SET             = 0x2;
/* Clear the aspect for this process */
constexpr usize PR_PPC_DEXCR_CTRL_CLEAR           = 0x4;
/* Set the aspect on exec */
constexpr usize PR_PPC_DEXCR_CTRL_SET_ONEXEC      = 0x8;
/* Clear the aspect on exec */
constexpr usize PR_PPC_DEXCR_CTRL_CLEAR_ONEXEC    = 0x10;
constexpr usize PR_PPC_DEXCR_CTRL_MASK            = 0x1f;

/*
 * Get the current shadow stack configuration for the current thread,
 * this will be the value configured via PR_SET_SHADOW_STACK_STATUS.
 */
constexpr usize PR_GET_SHADOW_STACK_STATUS        = 74;

/*
 * Set the current shadow stack configuration.  Enabling the shadow
 * stack will cause a shadow stack to be allocated for the thread.
 */
constexpr usize PR_SET_SHADOW_STACK_STATUS        = 75;
constexpr usize PR_SHADOW_STACK_ENABLE            = Bit(0);
constexpr usize PR_SHADOW_STACK_WRITE             = Bit(1);
constexpr usize PR_SHADOW_STACK_PUSH              = Bit(2);

/*
 * Prevent further changes to the specified shadow stack
 * configuration.  All bits may be locked via this call, including
 * undefined bits.
 */
constexpr usize PR_LOCK_SHADOW_STACK_STATUS       = 76;

/*
 * Controls the mode of timer_create() for CRIU restore operations.
 * Enabling this allows CRIU to restore timers with explicit IDs.
 *
 * Don't use for normal operations as the result might be undefined.
 */
constexpr usize PR_TIMER_CREATE_RESTORE_IDS       = 77;
constexpr usize PR_TIMER_CREATE_RESTORE_IDS_OFF   = 0;
constexpr usize PR_TIMER_CREATE_RESTORE_IDS_ON    = 1;
constexpr usize PR_TIMER_CREATE_RESTORE_IDS_GET   = 2;

/* FUTEX hash management */
constexpr usize PR_FUTEX_HASH                     = 78;
constexpr usize PR_FUTEX_HASH_SET_SLOTS           = 1;
constexpr usize FH_FLAG_IMMUTABLE                 = Bit(0);
constexpr usize PR_FUTEX_HASH_GET_SLOTS           = 2;
constexpr usize PR_FUTEX_HASH_GET_IMMUTABLE       = 3;
