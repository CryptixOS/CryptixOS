/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/linux/sched.h>
#include <Arch/CPU.hpp>
#include <Arch/InterruptGuard.hpp>

#include <Library/StackBuilder.hpp>
#include <Memory/PMM.hpp>

#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

struct CTOS_PACKED SignalFrame
{
    upointer         SignalTrampoline;
    ExecutionContext Context;
};

extern KeyValuePair<Pointer, usize> SignalTrampoline();
Thread::Thread(Process* parent, Pointer pc, Pointer arg, i64 runOn, bool user)
    : m_State(ThreadState::eDequeued)
    , m_ErrorCode(no_error)
    , m_Parent(parent)
    , m_IsUser(user)
    , m_IsEnqueued(false)

{
    m_ID = parent->m_NextTid++;
    CPU::PrepareThread(this, pc, arg);
    m_Tls.Self      = this;
    m_Tls.RunningOn = runOn;
}

Thread* Thread::Current() { return CPU::GetCurrentThread(); }

void    Thread::SetRunningOn(isize runningOn) { m_Tls.RunningOn = runningOn; }

Pointer Thread::GetStack() const { return m_Tls.Stack; }
void    Thread::SetStack(Pointer stack) { m_Tls.Stack = stack; }

Pointer Thread::PageFaultStack() const { return m_Tls.PageFaultStack; }
Pointer Thread::KernelStack() const { return m_Tls.KernelStack; }

void    Thread::SetPageFaultStack(Pointer pfstack)
{
    m_Tls.PageFaultStack = pfstack;
}
void    Thread::SetKernelStack(Pointer kstack) { m_Tls.KernelStack = kstack; }

Pointer Thread::FpuStorage() const { return m_Tls.FpuStorage; }
void    Thread::SetFpuStorage(Pointer fpuStorage, usize pageCount)
{
    m_Tls.FpuStorage          = fpuStorage;
    m_Tls.FpuStoragePageCount = pageCount;
}

Thread::Thread(Process* parent, Vector<StringView>& argv,
               Vector<StringView>& envp, ExecutableProgram& program, i64 runOn)
    : m_State(ThreadState::eDequeued)
    , m_ErrorCode(no_error)
    , m_Parent(parent)
    , m_IsUser(true)
    , m_IsEnqueued(false)
{
    m_Tls.RunningOn = CPU::Current()->ID;
    m_Tls.Self      = this;
    m_ID            = parent->m_NextTid++;

    if (!parent->PageMap) parent->PageMap = VMM::GetKernelPageMap();

    auto [stackTopWritable, stackTopVirt] = AllocateUserStack();
    m_Tls.Stack
        = program.PrepareStack(stackTopWritable, stackTopVirt, argv, envp);
    m_Parent->m_SignalTrampolineVirt = program.SignalTrampoline();

    CPU::PrepareThread(this, program.EntryPoint(), 0);
}

Thread::~Thread()
{
    PMM::FreePages(m_Tls.FpuStorage, m_Tls.FpuStoragePageCount);
    PMM::FreePages(m_Tls.KernelStack,
                   Math::DivRoundUp(CPU::KERNEL_STACK_SIZE, PMM::PAGE_SIZE));
    PMM::FreePages(m_Tls.PageFaultStack,
                   Math::DivRoundUp(CPU::KERNEL_STACK_SIZE, PMM::PAGE_SIZE));
}

void Thread::OnSyscallEnter() { m_ExecutingSyscall = true; }
void Thread::OnSyscallLeave() { m_ExecutingSyscall = false; }

void Thread::SendSignal(u8 signal)
{
    InterruptGuard guard(false);
    LogDebug(
        "Thread::SendSignal: Sending signal #{}({}) to process {}\nIgnored: {}",
        signal, StringUtils::ToString(static_cast<SignalID>(signal)),
        m_Parent->ID(), ShouldIgnoreSignal(signal));

    if (!signal)
    {
        LogDebug("Thread::SendSignal: Dummy signal, returning...");
        // return;
    }
    if (ShouldIgnoreSignal(signal)) return;
    m_PendingSignals.Add(signal);
    if (m_State == ThreadState::eBlocked)
        ;
}
bool Thread::DispatchAnyPendingSignal()
{
    // FIXME(v1tr10l7): aarch64 implementation
    // if (m_ExecutingSyscall) return false;
#if CTOS_TARGET_X86_64
    if (Context.CS == GDT::KERNEL_CODE_SELECTOR
        || Context.DS == GDT::KERNEL_DATA_SELECTOR)
        return false;
#endif

    Assert(!CPU::GetInterruptFlag());

    usize i = 0;
    for (i = 0; i < 32; i++)
        if (m_PendingSignals.Contains(i))
            LogDebug(
                "Thread::DispatchAnyPendingSignal: Found pending signal => {}",
                i);
    auto pendingSignals = m_PendingSignals & ~m_SignalMask;
    if (i != 32) LogDebug("Still pending => {}", pendingSignals.Contains(i));
    u8 signal = 0;
    while (signal < 32 && !(pendingSignals.Contains(signal))) ++signal;
    if (signal == 32) return false;

    return DispatchSignal(signal);
}

extern Pointer g_SignalTrampoline;
bool           Thread::DispatchSignal(u8 signal)
{
    Assert(!CPU::GetInterruptFlag());
    Assert(signal < 32);
    m_PendingSignals.Remove(signal);

    LogDebug("Thread: Context => \n{}", Context);
#if CTOS_TARGET_X86_64
    Assert(Context.SS == (GDT::USERLAND_DATA_SELECTOR | 0x03));

    auto& action = m_Parent->SignalAction(static_cast<SignalID>(signal));
    if (action.VirtualAddress)
    {
        // Skip the interrupted code's red zone (128 bytes below rsp), then
        // build the frame through the user virtual address: the scheduler
        // has already loaded this thread's page map. (Translating through the
        // stack region's physical base assumed one contiguous main stack and
        // wrote the frame elsewhere for any other stack, so sigreturn
        // restored an all-zero context.)
        constexpr usize RED_ZONE_SIZE = 128;
        auto            rsp           = Context.RSP - RED_ZONE_SIZE;
        Pointer         rspWritable   = Pointer(rsp);
        StackBuilder    builder(rspWritable);

        upointer     trampolineVirt = m_Parent->m_SignalTrampolineVirt;
        {
            UserMemoryProtectionGuard guard;

            usize fpuStorageSize = m_Tls.FpuStoragePageCount * PMM::PAGE_SIZE;
            upointer contextAddress = builder.Write(&Context, sizeof(Context));
            LogDebug("Thread: Saved the thread's context to => {:#x}",
                     contextAddress);

            upointer fpuAddress
                = builder.Write(m_Tls.FpuStorage, fpuStorageSize);

            for (usize i = 0; i < 128 / 8; i++) builder.Write(0);

            // Frame, from the handler's entry rsp up:
            //   [rsp]      trampoline (the handler's return address)
            //   [rsp + 8]  address of the saved context
            //   [rsp + 16] address of the saved fpu state
            // The handler returning pops the first, so sigreturn's rsp
            // points straight at the two addresses: no offsets that depend
            // on alignment padding. rsp + 8 is 16-aligned, as the ABI wants
            // ((rsp + 8) % 16 == 0 at function entry).
            builder.Align(16);
            builder.Write(fpuAddress);
            builder.Write(contextAddress);
            builder.Write(trampolineVirt);

            rsp -= (builder.Top() - builder.Current()).Raw();
        }

        Context.RSP = rsp;
        Context.RIP = action.VirtualAddress;
        Context.RDI = signal;
        return true;
    }

    switch (signal)
    {
        case SIGHUP:
        case SIGINT:
        case SIGKILL:
        case SIGPIPE:
        case SIGALRM:
        case SIGUSR1:
        case SIGUSR2:
        case SIGVTALRM:
        case SIGSTKFLT:
        case SIGIO:
        case SIGPROF:
        case SIGTERM:
            // TODO(v1tr10l7): Terminate
            if (m_Parent->ID() != m_Parent->m_Credentials.ProcessGroupID)
                m_Parent->Exit(0);
            break;
        case SIGCHLD:
        case SIGURG:
        case SIGWINCH:
            // TODO(v1tr10l7): Ignore
            break;
        case SIGQUIT:
        case SIGILL:
        case SIGTRAP:
        case SIGABRT:
        case SIGBUS:
        case SIGFPE:
        case SIGSEGV:
        case SIGXCPU:
        case SIGXFSZ:
            // TODO(v1tr10l7): Dump Core
            break;
        case SIGCONT:
            // TODO(v1trSystem Management Bus - Wikipedia10l7): Continue
            break;
        case SIGSTOP:
        case SIGTSTP:
        case SIGTTIN:
        case SIGTTOU:
            // TODO(v1tr10l7): Stop
            break;

        default: AssertNotReached(); break;
    }
        // TODO(v1tr10l7): Dispatch signals
#endif
    return false;
}
ErrorOr<void> Thread::SignalReturn()
{
#if CTOS_TARGET_X86_64
    // The frame sits above the stack pointer the trampoline made the
    // sigreturn syscall with. Context holds wherever the thread was last
    // preempted, which is deeper inside the handler if a tick hit it.
    Pointer rsp = SavedContext.RSP;
    {
        UserMemoryProtectionGuard guard;

        usize fpuStorageSize = m_Tls.FpuStoragePageCount * PMM::PAGE_SIZE;
        // See DispatchSignal() for the layout
        auto  slots          = rsp.As<upointer>();
        auto  saved          = reinterpret_cast<ExecutionContext*>(slots[0]);
        auto  fpuState       = reinterpret_cast<void*>(slots[1]);
        auto  current        = Process::Current();
        if (!current->ValidateRead(saved, sizeof(ExecutionContext))
            || !current->ValidateRead(fpuState, fpuStorageSize))
            return Error(EFAULT);

        Memory::Copy(m_Tls.FpuStorage, fpuState, fpuStorageSize);
        LogWarn("SigReturn: Saved stack frame dump =>\n{}", *saved);
        Context = *saved;

        // The frame is user memory: never return to it with kernel segments
        // or privileged flags, whatever it contains
        constexpr u64 USER_FLAGS = 0x50dd5; // AC RF OF DF TF SF ZF AF PF CF
        Context.CS     = GDT::USERLAND_CODE_SELECTOR | 0x03;
        Context.SS     = GDT::USERLAND_DATA_SELECTOR | 0x03;
        Context.DS     = GDT::USERLAND_DATA_SELECTOR | 0x03;
        Context.ES     = GDT::USERLAND_DATA_SELECTOR | 0x03;
        Context.RFlags = (Context.RFlags & USER_FLAGS) | 0x202; // IF, bit 1
        return {};
    }

#endif
    for (;;) Arch::Halt();
    return Error(ENOSYS);
}

ErrorOr<::Ref<Thread>> Thread::Clone(usize flags, Pointer stack,
                                     CTOS_UNUSED usize stackSize, Pointer tls)
{

    if ((flags & (CLONE_NEWNS | CLONE_FS)) == (CLONE_NEWNS | CLONE_FS))
        return Error(EINVAL);
    if ((flags & (CLONE_NEWUSER | CLONE_FS)) == (CLONE_NEWUSER | CLONE_FS))
        return Error(EINVAL);

    if ((flags & CLONE_THREAD) && !(flags & CLONE_SIGHAND))
        return Error(EINVAL);
    if ((flags & CLONE_SIGHAND) && !(flags & CLONE_VM)) return Error(EINVAL);
    if (flags & (CLONE_PIDFD | CLONE_DETACHED)) return Error(EINVAL);

    auto parent = m_Parent;
    if (!(flags & CLONE_THREAD))
    {
        auto newProcess = TryOrRet(parent->Clone(flags));
        parent          = newProcess;
    }

    auto newThread = parent->CreateThread(Context.ProgramCounter, m_IsUser,
                                          CPU::GetCurrent()->ID);
    newThread->m_Tls.Self = newThread.Raw();
    if (!(flags & CLONE_VM))
    {
        stack     = m_Tls.Stack;
        stackSize = CPU::USER_STACK_SIZE;
    }

    newThread->m_Tls.Stack = stack;

    if (!(flags & CLONE_THREAD))
    {
        for (const auto& stack : m_Stacks)
        {
            usize stackVirt = stack->VirtualBase();

            auto  region    = parent->m_AddressSpace->Find(stackVirt);
            if (!region) continue;
            newThread->m_Stacks.PushBack(region);
        }
    }

    Memory::Copy(newThread->m_Tls.FpuStorage, m_Tls.FpuStorage,
                 m_Tls.FpuStoragePageCount * PMM::PAGE_SIZE);

    newThread->m_Parent     = parent;
    newThread->Context      = SavedContext;
    newThread->m_IsEnqueued = false;
#if CTOS_TARGET_X86_64
    newThread->Context.RAX    = 0;
    newThread->Context.RFlags = 0x202;
    newThread->Context.RDX    = 0;
    if (flags & CLONE_VM) newThread->Context.RSP = stack;

    newThread->m_GsBase = m_GsBase;
    newThread->m_FsBase = m_FsBase;

    if (flags & CLONE_SETTLS) newThread->m_FsBase = tls;
#endif

    newThread->m_State = ThreadState::eDequeued;
    return newThread;
}

void Thread::SetSignalMask(SignalSet mask)
{
    if (m_SignalMask == mask) return;

    ScopedLock guard(m_SignalMaskLock, true);
    m_SignalMask = mask;
}

KeyValuePair<upointer, upointer> Thread::AllocateUserStack()
{
    usize   pageCount = CPU::USER_STACK_SIZE / PMM::PAGE_SIZE + 1;
    Pointer stackPhys = PMM::CallocatePages(pageCount);

    Pointer guardVirt
        = m_Parent->m_UserStackTop.Offset(-pageCount * PMM::PAGE_SIZE);
    Pointer stackVirt = guardVirt.Offset(PMM::PAGE_SIZE);

    Assert(m_Parent->PageMap->MapRange(
        stackVirt, stackPhys, CPU::USER_STACK_SIZE,
        PageAttributes::eRWXU | PageAttributes::eWriteBack));

    using VMM::Access;
    auto stackRegion = new Region(stackPhys, stackVirt, CPU::USER_STACK_SIZE);
    stackRegion->SetAccessMode(Access::eReadWriteExecute | Access::eUser);
    m_Stacks.PushBack(stackRegion);
    m_Parent->m_AddressSpace->Insert(stackVirt, stackRegion);

    m_StackVirt              = stackVirt;
    m_Parent->m_UserStackTop = guardVirt.Raw() - PMM::PAGE_SIZE;
    return {stackPhys.ToHigherHalf<Pointer>().Offset(CPU::USER_STACK_SIZE),
            stackVirt.Offset(CPU::USER_STACK_SIZE)};
};
