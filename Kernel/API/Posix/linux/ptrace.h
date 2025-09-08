/*
 * Created by v1tr10l7 on 08.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

constexpr usize PTRACE_TRACEME     = 0;
constexpr usize PTRACE_PEEKTEXT    = 1;
constexpr usize PTRACE_PEEKDATA    = 2;
constexpr usize PTRACE_PEEKUSR     = 3;
constexpr usize PTRACE_POKETEXT    = 4;
constexpr usize PTRACE_POKEDATA    = 5;
constexpr usize PTRACE_POKEUSR     = 6;
constexpr usize PTRACE_CONT        = 7;
constexpr usize PTRACE_KILL        = 8;
constexpr usize PTRACE_SINGLESTEP  = 9;

constexpr usize PTRACE_ATTACH      = 16;
constexpr usize PTRACE_DETACH      = 17;

constexpr usize PTRACE_SYSCALL     = 24;

/* 0x4200-0x4300 are reserved for architecture-independent additions.  */
constexpr usize PTRACE_SETOPTIONS  = 0x4200;
constexpr usize PTRACE_GETEVENTMSG = 0x4201;
constexpr usize PTRACE_GETSIGINFO  = 0x4202;
constexpr usize PTRACE_SETSIGINFO  = 0x4203;

/*
 * Generic ptrace interface that exports the architecture specific regsets
 * using the corresponding NT_* types (which are also used in the core dump).
 * Please note that the NT_PRSTATUS note type in a core dump contains a full
 * 'struct elf_prstatus'. But the user_regset for NT_PRSTATUS contains just the
 * elf_gregset_t that is the pr_reg field of 'struct elf_prstatus'. For all the
 * other user_regset flavors, the user_regset layout and the ELF core dump note
 * payload are exactly the same layout.
 *
 * This interface usage is as follows:
 *	struct iovec iov = { buf, len};
 *
 *	ret = ptrace(PTRACE_GETREGSET/PTRACE_SETREGSET, pid, NT_XXX_TYPE, &iov);
 *
 * On the successful completion, iov.len will be updated by the kernel,
 * specifying how much the kernel has written/read to/from the user's iov.buf.
 */
constexpr usize PTRACE_GETREGSET   = 0x4204;
constexpr usize PTRACE_SETREGSET   = 0x4205;

constexpr usize PTRACE_SEIZE       = 0x4206;
constexpr usize PTRACE_INTERRUPT   = 0x4207;
constexpr usize PTRACE_LISTEN      = 0x4208;

constexpr usize PTRACE_PEEKSIGINFO = 0x4209;

struct ptrace_peeksiginfo_args
{
    /* from which siginfo to start */
    u64 off;
    u32 flags;
    /* how may siginfos to take */
    i32 nr;
};

constexpr usize PTRACE_GETSIGMASK           = 0x420a;
constexpr usize PTRACE_SETSIGMASK           = 0x420b;

constexpr usize PTRACE_SECCOMP_GET_FILTER   = 0x420c;
constexpr usize PTRACE_SECCOMP_GET_METADATA = 0x420d;

struct seccomp_metadata
{
    /* Input: which filter */
    u64 filter_off;
    /* Output: filter's flags */
    u64 flags;
};

constexpr usize PTRACE_GET_SYSCALL_INFO     = 0x420e;
constexpr usize PTRACE_SET_SYSCALL_INFO     = 0x4212;
constexpr usize PTRACE_SYSCALL_INFO_NONE    = 0;
constexpr usize PTRACE_SYSCALL_INFO_ENTRY   = 1;
constexpr usize PTRACE_SYSCALL_INFO_EXIT    = 2;
constexpr usize PTRACE_SYSCALL_INFO_SECCOMP = 3;

struct ptrace_syscall_info
{
    /* PTRACE_SYSCALL_INFO_* */
    u8  op;
    u8  reserved;
    u16 flags;
    u32 arch;
    u64 instruction_pointer;
    u64 stack_pointer;
    union
    {
        struct
        {
            u64 nr;
            u64 args[6];
        } entry;
        struct
        {
            i64 rval;
            u8  is_error;
        } exit;
        struct
        {
            u64 nr;
            u64 args[6];
            u32 ret_data;
            u32 reserved2;
        } seccomp;
    };
};

constexpr usize PTRACE_GET_RSEQ_CONFIGURATION = 0x420f;

struct ptrace_rseq_configuration
{
    u64 rseq_abi_pointer;
    u32 rseq_abi_size;
    u32 signature;
    u32 flags;
    u32 pad;
};

constexpr usize PTRACE_SET_SYSCALL_USER_DISPATCH_CONFIG = 0x4210;
constexpr usize PTRACE_GET_SYSCALL_USER_DISPATCH_CONFIG = 0x4211;

/*
 * struct ptrace_sud_config - Per-task configuration for Syscall User Dispatch
 * @mode:	One of PR_SYS_DISPATCH_ON or PR_SYS_DISPATCH_OFF
 * @selector:	Tracees user virtual address of SUD selector
 * @offset:	SUD exclusion area (virtual address)
 * @len:	Length of SUD exclusion area
 *
 * Used to get/set the syscall user dispatch configuration for a tracee.
 * Selector is optional (may be NULL), and if invalid will produce
 * a SIGSEGV in the tracee upon first access.
 *
 * If mode is PR_SYS_DISPATCH_ON, syscall dispatch will be enabled. If
 * PR_SYS_DISPATCH_OFF, syscall dispatch will be disabled and all other
 * parameters must be 0.  The value in *selector (if not null), also determines
 * whether syscall dispatch will occur.
 *
 * The Syscall User Dispatch Exclusion area described by offset/len is the
 * virtual address space from which syscalls will not produce a user
 * dispatch.
 */
struct ptrace_sud_config
{
    u64 mode;
    u64 selector;
    u64 offset;
    u64 len;
};

/* 0x4212 is PTRACE_SET_SYSCALL_INFO */
/*
 * These values are stored in task->ptrace_message
 * by ptrace_stop to describe the current syscall-stop.
 */
constexpr usize PTRACE_EVENTMSG_SYSCALL_ENTRY = 1;
constexpr usize PTRACE_EVENTMSG_SYSCALL_EXIT  = 2;

/* Read signals from a shared (process wide) queue */
constexpr usize PTRACE_PEEKSIGINFO_SHARED     = Bit(0);

/* Wait extended result codes for the above trace options.  */
constexpr usize PTRACE_EVENT_FORK             = 1;
constexpr usize PTRACE_EVENT_VFORK            = 2;
constexpr usize PTRACE_EVENT_CLONE            = 3;
constexpr usize PTRACE_EVENT_EXEC             = 4;
constexpr usize PTRACE_EVENT_VFORK_DONE       = 5;
constexpr usize PTRACE_EVENT_EXIT             = 6;
constexpr usize PTRACE_EVENT_SECCOMP          = 7;
/* Extended result codes which enabled by means other than options.  */
constexpr usize PTRACE_EVENT_STOP             = 128;

/* Options set using PTRACE_SETOPTIONS or using PTRACE_SEIZE @data param */
constexpr usize PTRACE_O_TRACESYSGOOD         = 1;
constexpr usize PTRACE_O_TRACEFORK            = Bit(PTRACE_EVENT_FORK);
constexpr usize PTRACE_O_TRACEVFORK           = Bit(PTRACE_EVENT_VFORK);
constexpr usize PTRACE_O_TRACECLONE           = Bit(PTRACE_EVENT_CLONE);
constexpr usize PTRACE_O_TRACEEXEC            = Bit(PTRACE_EVENT_EXEC);
constexpr usize PTRACE_O_TRACEVFORKDONE       = Bit(PTRACE_EVENT_VFORK_DONE);
constexpr usize PTRACE_O_TRACEEXIT            = Bit(PTRACE_EVENT_EXIT);
constexpr usize PTRACE_O_TRACESECCOMP         = Bit(PTRACE_EVENT_SECCOMP);

/* eventless options */
constexpr usize PTRACE_O_EXITKILL             = Bit(20);
constexpr usize PTRACE_O_SUSPEND_SECCOMP      = Bit(21);

constexpr usize PTRACE_O_MASK
    = (0x000000ff | PTRACE_O_EXITKILL | PTRACE_O_SUSPEND_SECCOMP);

/* Some constant macros are used in both assembler and
 * C code.  Therefore we cannot annotate them always with
 * 'UL' and other type specifiers unilaterally.  We
 * use the following macros to deal with this.
 *
 * Similarly, _AT() will cast an expression with a type in C, but
 * leave it unchanged in asm.
 */

#define __AC(X, Y)                   (X##Y)
#define _AC(X, Y)                    __AC(X, Y)
#define _AT(T, X)                    ((T)(X))

#define _UL(x)                       (_AC(x, ul))
#define _ULL(x)                      (_AC(x, ull))

#define _BITUL(x)                    (_UL(1) << (x))
#define _BITULL(x)                   (_ULL(1) << (x))

/*
 * Missing __asm__ support
 *
 * __BIT128() would not work in the __asm__ code, as it shifts an
 * 'unsigned __int128' data type as direct representation of
 * 128 bit constants is not supported in the gcc compiler, as
 * they get silently truncated.
 *
 * TODO: Please revisit this implementation when gcc compiler
 * starts representing 128 bit constants directly like long
 * and unsigned long etc. Subsequently drop the comment for
 * GENMASK_U128() which would then start supporting __asm__ code.
 */
#define _BIT128(x)                   ((unsigned __int128)(1) << (x))

#define __ALIGN_KERNEL(x, a)         __ALIGN_KERNEL_MASK(x, (__typeof__(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask) (((x) + (mask)) & ~(mask))

#define __KERNEL_DIV_ROUND_UP(n, d)  (((n) + (d) - 1) / (d))

/*
 * EFLAGS bits
 */
/* Carry Flag */
constexpr usize X86_EFLAGS_CF_BIT    = 0;
constexpr usize X86_EFLAGS_CF        = _BITUL(X86_EFLAGS_CF_BIT);
/* Bit 1 - always on */
constexpr usize X86_EFLAGS_FIXED_BIT = 1;
constexpr usize X86_EFLAGS_FIXED     = _BITUL(X86_EFLAGS_FIXED_BIT);
/* Parity Flag */
constexpr usize X86_EFLAGS_PF_BIT    = 2;
constexpr usize X86_EFLAGS_PF        = _BITUL(X86_EFLAGS_PF_BIT);
/* Auxiliary carry Flag */
constexpr usize X86_EFLAGS_AF_BIT    = 4;
constexpr usize X86_EFLAGS_AF        = _BITUL(X86_EFLAGS_AF_BIT);
/* Zero Flag */
constexpr usize X86_EFLAGS_ZF_BIT    = 6;
constexpr usize X86_EFLAGS_ZF        = _BITUL(X86_EFLAGS_ZF_BIT);
/* Sign Flag */
constexpr usize X86_EFLAGS_SF_BIT    = 7;
constexpr usize X86_EFLAGS_SF        = _BITUL(X86_EFLAGS_SF_BIT);
/* Trap Flag */
constexpr usize X86_EFLAGS_TF_BIT    = 8;
constexpr usize X86_EFLAGS_TF        = _BITUL(X86_EFLAGS_TF_BIT);
/* Interrupt Flag */
constexpr usize X86_EFLAGS_IF_BIT    = 9;
constexpr usize X86_EFLAGS_IF        = _BITUL(X86_EFLAGS_IF_BIT);
/* Direction Flag */
constexpr usize X86_EFLAGS_DF_BIT    = 10;
constexpr usize X86_EFLAGS_DF        = _BITUL(X86_EFLAGS_DF_BIT);
/* Overflow Flag */
constexpr usize X86_EFLAGS_OF_BIT    = 11;
constexpr usize X86_EFLAGS_OF        = _BITUL(X86_EFLAGS_OF_BIT);
/* I/O Privilege Level (2 bits) */
constexpr usize X86_EFLAGS_IOPL_BIT  = 12;
constexpr usize X86_EFLAGS_IOPL      = (_AC(3, UL) << X86_EFLAGS_IOPL_BIT);
/* Nested Task */
constexpr usize X86_EFLAGS_NT_BIT    = 14;
constexpr usize X86_EFLAGS_NT        = _BITUL(X86_EFLAGS_NT_BIT);
/* Resume Flag */
constexpr usize X86_EFLAGS_RF_BIT    = 16;
constexpr usize X86_EFLAGS_RF        = _BITUL(X86_EFLAGS_RF_BIT);
/* Virtual Mode */
constexpr usize X86_EFLAGS_VM_BIT    = 17;
constexpr usize X86_EFLAGS_VM        = _BITUL(X86_EFLAGS_VM_BIT);
/* Alignment Check/Access Control */
constexpr usize X86_EFLAGS_AC_BIT    = 18;
constexpr usize X86_EFLAGS_AC        = _BITUL(X86_EFLAGS_AC_BIT);
/* Virtual Interrupt Flag */
constexpr usize X86_EFLAGS_VIF_BIT   = 19;
constexpr usize X86_EFLAGS_VIF       = _BITUL(X86_EFLAGS_VIF_BIT);
/* Virtual Interrupt Pending */
constexpr usize X86_EFLAGS_VIP_BIT   = 20;
constexpr usize X86_EFLAGS_VIP       = _BITUL(X86_EFLAGS_VIP_BIT);
/* CPUID detection */
constexpr usize X86_EFLAGS_ID_BIT    = 21;
constexpr usize X86_EFLAGS_ID        = _BITUL(X86_EFLAGS_ID_BIT);

/*
 * Basic CPU control in CR0
 */
/* Protection Enable */
constexpr usize X86_CR0_PE_BIT       = 0;
constexpr usize X86_CR0_PE           = _BITUL(X86_CR0_PE_BIT);
/* Monitor Coprocessor */
constexpr usize X86_CR0_MP_BIT       = 1;
constexpr usize X86_CR0_MP           = _BITUL(X86_CR0_MP_BIT);
/* Emulation */
constexpr usize X86_CR0_EM_BIT       = 2;
constexpr usize X86_CR0_EM           = _BITUL(X86_CR0_EM_BIT);
/* Task Switched */
constexpr usize X86_CR0_TS_BIT       = 3;
constexpr usize X86_CR0_TS           = _BITUL(X86_CR0_TS_BIT);
/* Extension Type */
constexpr usize X86_CR0_ET_BIT       = 4;
constexpr usize X86_CR0_ET           = _BITUL(X86_CR0_ET_BIT);
/* Numeric Error */
constexpr usize X86_CR0_NE_BIT       = 5;
constexpr usize X86_CR0_NE           = _BITUL(X86_CR0_NE_BIT);
/* Write Protect */
constexpr usize X86_CR0_WP_BIT       = 16;
constexpr usize X86_CR0_WP           = _BITUL(X86_CR0_WP_BIT);
/* Alignment Mask */
constexpr usize X86_CR0_AM_BIT       = 18;
constexpr usize X86_CR0_AM           = _BITUL(X86_CR0_AM_BIT);
/* Not Write-through */
constexpr usize X86_CR0_NW_BIT       = 29;
constexpr usize X86_CR0_NW           = _BITUL(X86_CR0_NW_BIT);
/* Cache Disable */
constexpr usize X86_CR0_CD_BIT       = 30;
constexpr usize X86_CR0_CD           = _BITUL(X86_CR0_CD_BIT);
/* Paging */
constexpr usize X86_CR0_PG_BIT       = 31;
constexpr usize X86_CR0_PG           = _BITUL(X86_CR0_PG_BIT);

/*
 * Paging options in CR3
 */
/* Page Write Through */
constexpr usize X86_CR3_PWT_BIT      = 3;
constexpr usize X86_CR3_PWT          = _BITUL(X86_CR3_PWT_BIT);
/* Page Cache Disable */
constexpr usize X86_CR3_PCD_BIT      = 4;
constexpr usize X86_CR3_PCD          = _BITUL(X86_CR3_PCD_BIT);

constexpr usize X86_CR3_PCID_BITS    = 12;
constexpr usize X86_CR3_PCID_MASK   = (_AC((1UL << X86_CR3_PCID_BITS) - 1, ul));

/* Activate LAM for userspace, 62:57 bits masked */
constexpr usize X86_CR3_LAM_U57_BIT = 61;
constexpr usize X86_CR3_LAM_U57     = _BITULL(X86_CR3_LAM_U57_BIT);
/* Activate LAM for userspace, 62:48 bits masked */
constexpr usize X86_CR3_LAM_U48_BIT = 62;
constexpr usize X86_CR3_LAM_U48     = _BITULL(X86_CR3_LAM_U48_BIT);
/* Preserve old PCID */
constexpr usize X86_CR3_PCID_NOFLUSH_BIT = 63;
constexpr usize X86_CR3_PCID_NOFLUSH     = _BITULL(X86_CR3_PCID_NOFLUSH_BIT);

/*
 * Intel CPU features in CR4
 */
/* enable vm86 extensions */
constexpr usize X86_CR4_VME_BIT          = 0;
constexpr usize X86_CR4_VME              = _BITUL(X86_CR4_VME_BIT);
/* virtual interrupts flag enable */
constexpr usize X86_CR4_PVI_BIT          = 1;
constexpr usize X86_CR4_PVI              = _BITUL(X86_CR4_PVI_BIT);
/* disable time stamp at ipl 3 */
constexpr usize X86_CR4_TSD_BIT          = 2;
constexpr usize X86_CR4_TSD              = _BITUL(X86_CR4_TSD_BIT);
/* enable debugging extensions */
constexpr usize X86_CR4_DE_BIT           = 3;
constexpr usize X86_CR4_DE               = _BITUL(X86_CR4_DE_BIT);
/* enable page size extensions */
constexpr usize X86_CR4_PSE_BIT          = 4;
constexpr usize X86_CR4_PSE              = _BITUL(X86_CR4_PSE_BIT);
/* enable physical address extensions */
constexpr usize X86_CR4_PAE_BIT          = 5;
constexpr usize X86_CR4_PAE              = _BITUL(X86_CR4_PAE_BIT);
/* Machine check enable */
constexpr usize X86_CR4_MCE_BIT          = 6;
constexpr usize X86_CR4_MCE              = _BITUL(X86_CR4_MCE_BIT);
/* enable global pages */
constexpr usize X86_CR4_PGE_BIT          = 7;
constexpr usize X86_CR4_PGE              = _BITUL(X86_CR4_PGE_BIT);
/* enable performance counters at ipl 3 */
constexpr usize X86_CR4_PCE_BIT          = 8;
constexpr usize X86_CR4_PCE              = _BITUL(X86_CR4_PCE_BIT);
/* enable fast FPU save and restore */
constexpr usize X86_CR4_OSFXSR_BIT       = 9;
constexpr usize X86_CR4_OSFXSR           = _BITUL(X86_CR4_OSFXSR_BIT);
/* enable unmasked SSE exceptions */
constexpr usize X86_CR4_OSXMMEXCPT_BIT   = 10;
constexpr usize X86_CR4_OSXMMEXCPT       = _BITUL(X86_CR4_OSXMMEXCPT_BIT);
/* enable UMIP support */
constexpr usize X86_CR4_UMIP_BIT         = 11;
constexpr usize X86_CR4_UMIP             = _BITUL(X86_CR4_UMIP_BIT);
/* enable 5-level page tables */
constexpr usize X86_CR4_LA57_BIT         = 12;
constexpr usize X86_CR4_LA57             = _BITUL(X86_CR4_LA57_BIT);
/* enable VMX virtualization */
constexpr usize X86_CR4_VMXE_BIT         = 13;
constexpr usize X86_CR4_VMXE             = _BITUL(X86_CR4_VMXE_BIT);
/* enable safer mode (TXT) */
constexpr usize X86_CR4_SMXE_BIT         = 14;
constexpr usize X86_CR4_SMXE             = _BITUL(X86_CR4_SMXE_BIT);
/* enable RDWRFSGS support */
constexpr usize X86_CR4_FSGSBASE_BIT     = 16;
constexpr usize X86_CR4_FSGSBASE         = _BITUL(X86_CR4_FSGSBASE_BIT);
/* enable PCID support */
constexpr usize X86_CR4_PCIDE_BIT        = 17;
constexpr usize X86_CR4_PCIDE            = _BITUL(X86_CR4_PCIDE_BIT);
/* enable xsave and xrestore */
constexpr usize X86_CR4_OSXSAVE_BIT      = 18;
constexpr usize X86_CR4_OSXSAVE          = _BITUL(X86_CR4_OSXSAVE_BIT);
/* enable SMEP support */
constexpr usize X86_CR4_SMEP_BIT         = 20;
constexpr usize X86_CR4_SMEP             = _BITUL(X86_CR4_SMEP_BIT);
/* enable SMAP support */
constexpr usize X86_CR4_SMAP_BIT         = 21;
constexpr usize X86_CR4_SMAP             = _BITUL(X86_CR4_SMAP_BIT);
/* enable Protection Keys support */
constexpr usize X86_CR4_PKE_BIT          = 22;
constexpr usize X86_CR4_PKE              = _BITUL(X86_CR4_PKE_BIT);
constexpr usize X86_CR4_CET_BIT          = 23;
constexpr usize X86_CR4_CET              = _BITUL(X86_CR4_CET_BIT);
/* LAM for supervisor pointers */
constexpr usize X86_CR4_LAM_SUP_BIT      = 28;
constexpr usize X86_CR4_LAM_SUP          = _BITUL(X86_CR4_LAM_SUP_BIT);

#ifdef __x86_64__
/* enable FRED kernel entry */
constexpr usize X86_CR4_FRED_BIT = 32;
constexpr usize X86_CR4_FRED     = _BITUL(X86_CR4_FRED_BIT);
#else
constexpr usize X86_CR4_FRED = (0);
#endif

/*
 * x86-64 Task Priority Register, CR8
 */
/* task priority register */
constexpr usize X86_CR8_TPR   = _AC(0x0000000f, ul);

/*
 * AMD and Transmeta use MSRs for configuration; see <asm/msr-index.h>
 */

/*
 *      NSC/Cyrix CPU configuration register indexes
 */
constexpr usize CX86_PCR0     = 0x20;
constexpr usize CX86_GCR      = 0xb8;
constexpr usize CX86_CCR0     = 0xc0;
constexpr usize CX86_CCR1     = 0xc1;
constexpr usize CX86_CCR2     = 0xc2;
constexpr usize CX86_CCR3     = 0xc3;
constexpr usize CX86_CCR4     = 0xe8;
constexpr usize CX86_CCR5     = 0xe9;
constexpr usize CX86_CCR6     = 0xea;
constexpr usize CX86_CCR7     = 0xeb;
constexpr usize CX86_PCR1     = 0xf0;
constexpr usize CX86_DIR0     = 0xfe;
constexpr usize CX86_DIR1     = 0xff;
constexpr usize CX86_ARR_BASE = 0xc4;
constexpr usize CX86_RCR_BASE = 0xdc;

constexpr usize CR0_STATE = (X86_CR0_PE | X86_CR0_MP | X86_CR0_ET | X86_CR0_NE
                             | X86_CR0_WP | X86_CR0_AM | X86_CR0_PG);

#ifdef __i386__

constexpr usize EBX = 0;
constexpr usize ECX = 1 constexpr usize EDX        = 2;
constexpr usize                         ESI        = 3;
constexpr usize                         EDI        = 4;
constexpr usize                         EBP        = 5;
constexpr usize                         EAX        = 6;
constexpr usize                         DS         = 7;
constexpr usize                         ES         = 8;
constexpr usize                         FS         = 9;
constexpr usize                         GS         = 10;
constexpr usize                         ORIG_EAX   = 11;
constexpr usize                         EIP        = 12;
constexpr usize                         CS         = 13;
constexpr usize                         EFL        = 14;
constexpr usize                         UESP       = 15;
constexpr usize                         SS         = 16;
constexpr usize                         FRAME_SIZE = 17;

#else /* __i386__ */

/*
 * C ABI says these regs are callee-preserved. They aren't saved on
 * kernel entry unless syscall needs a complete, fully filled "struct
 * pt_regs".
 */
constexpr usize R15        = 0;
constexpr usize R14        = 8;
constexpr usize R13        = 16;
constexpr usize R12        = 24;
constexpr usize RBP        = 32;
constexpr usize RBX        = 40;
/* These regs are callee-clobbered. Always saved on kernel entry. */
constexpr usize R11        = 48;
constexpr usize R10        = 56;
constexpr usize R9         = 64;
constexpr usize R8         = 72;
constexpr usize RAX        = 80;
constexpr usize RCX        = 88;
constexpr usize RDX        = 96;
constexpr usize RSI        = 104;
constexpr usize RDI        = 112;
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error
 * code. On hw interrupt, it's IRQ number:
 */
constexpr usize ORIG_RAX   = 120;
/* Return frame for iretq */
constexpr usize RIP        = 128;
constexpr usize CS         = 136;
constexpr usize EFLAGS     = 144;
constexpr usize RSP        = 152;
constexpr usize SS         = 160;

/* top of stack page */
constexpr usize FRAME_SIZE = 168;

#endif /* !__i386__ */

/* Arbitrarily choose the same ptrace numbers as used by the Sparc code. */
constexpr usize PTRACE_GETREGS         = 12;
constexpr usize PTRACE_SETREGS         = 13;
constexpr usize PTRACE_GETFPREGS       = 14;
constexpr usize PTRACE_SETFPREGS       = 15;
constexpr usize PTRACE_GETFPXREGS      = 18;
constexpr usize PTRACE_SETFPXREGS      = 19;

constexpr usize PTRACE_OLDSETOPTIONS   = 21;

/* only useful for access 32bit programs / kernels */
constexpr usize PTRACE_GET_THREAD_AREA = 25;
constexpr usize PTRACE_SET_THREAD_AREA = 26;

#ifdef __x86_64__
constexpr usize PTRACE_ARCH_PRCTL = 30;
#endif

constexpr usize PTRACE_SYSEMU            = 31;
constexpr usize PTRACE_SYSEMU_SINGLESTEP = 32;

/* resume execution until next branch */
constexpr usize PTRACE_SINGLEBLOCK       = 33;

#ifdef __i386__
/* this struct defines the way the registers are stored on the
   stack during a system call. */

struct pt_regs
{
    long ebx;
    long ecx;
    long edx;
    long esi;
    long edi;
    long ebp;
    long eax;
    int  xds;
    int  xes;
    int  xfs;
    int  xgs;
    long orig_eax;
    long eip;
    int  xcs;
    long eflags;
    long esp;
    int  xss;
};

#else /* __i386__ */

struct pt_regs
{
    /*
     * C ABI says these regs are callee-preserved. They aren't saved on kernel
     * entry unless syscall needs a complete, fully filled "struct pt_regs".
     */
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long rbp;
    unsigned long rbx;
    /* These regs are callee-clobbered. Always saved on kernel entry. */
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rax;
    unsigned long rcx;
    unsigned long rdx;
    unsigned long rsi;
    unsigned long rdi;
    /*
     * On syscall entry, this is syscall#. On CPU exception, this is error code.
     * On hw interrupt, it's IRQ number:
     */
    unsigned long orig_rax;
    /* Return frame for iretq */
    unsigned long rip;
    unsigned long cs;
    unsigned long eflags;
    unsigned long rsp;
    unsigned long ss;
    /* top of stack page */
};

#endif /* !__i386__ */
