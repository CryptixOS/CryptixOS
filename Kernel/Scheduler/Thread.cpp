/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/InterruptGuard.hpp>

#include <Library/StackBuilder.hpp>
#include <Memory/PMM.hpp>

#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

struct CTOS_PACKED SignalFrame
{
    upointer         SignalTrampoline;
    ExecutionContext Context;
};

extern KeyValuePair<Pointer, usize> SignalTrampoline();
Thread::Thread(Process* parent, Pointer pc, Pointer arg, i64 runOn)
    : m_State(ThreadState::eDequeued)
    , m_ErrorCode(no_error)
    , m_Parent(parent)
    , m_IsUser(false)
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

    if (ShouldIgnoreSignal(signal)) return;
    m_PendingSignals.Add(signal);
}
bool Thread::DispatchAnyPendingSignal()
{
    // FIXME(v1tr10l7): aarch64 implementation
    if (m_ExecutingSyscall) return false;
#if CTOS_TARGET_X86_64
    if (Context.CS == GDT::KERNEL_CODE_SELECTOR
        || Context.DS == GDT::KERNEL_DATA_SELECTOR)
        return false;
#endif

    Assert(!CPU::GetInterruptFlag());
    auto pendingSignals = m_PendingSignals & ~m_SignalMask;

    u8   signal         = 0;
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
        auto    rsp          = Context.RSP;
        Pointer phys         = nullptr;
        Pointer stackTopVirt = nullptr;

        for (auto [virt, region] : m_Parent->m_AddressSpace)
        {
            if (!region->Contains(rsp - 1)) continue;
            phys         = region->PhysicalBase();
            stackTopVirt = virt.Offset(CPU::USER_STACK_SIZE);
        }

        usize   stackOffset = CPU::USER_STACK_SIZE - (stackTopVirt.Raw() - rsp);
        Pointer rspWritable = phys.ToHigherHalf().Offset(stackOffset);
        StackBuilder builder(rspWritable);

        upointer     trampolineVirt = m_Parent->m_SignalTrampolineVirt;
        {
            CPU::UserMemoryProtectionGuard guard;

            auto actualPhys = Pointer(rspWritable).FromHigherHalf();
            LogDebug("Thread: Phys => {:#x}, ActualPhys => {:#x}", phys,
                     actualPhys);

            usize fpuStorageSize = m_Tls.FpuStoragePageCount * PMM::PAGE_SIZE;
            upointer contextAddress = builder.Write(&Context, sizeof(Context));
            LogDebug("Thread: Saved the thread's context to => {:#x}",
                     contextAddress);

            builder.Write(m_Tls.FpuStorage, fpuStorageSize);

            for (usize i = 0; i < 128 / 8; i++) builder.Write(0);
            builder.Align(16);
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
    Pointer rsp = Context.RSP;
    {
        CPU::UserMemoryProtectionGuard guard;

        usize fpuStorageSize = m_Tls.FpuStoragePageCount * PMM::PAGE_SIZE;
        Memory::Copy(m_Tls.FpuStorage, rsp.Offset(16 + 128), fpuStorageSize);

        ExecutionContext* saved
            = rsp.Offset<ExecutionContext*>(16 + 128 + fpuStorageSize);
        LogWarn("SigReturn: Saved stack frame dump =>\n{}", *saved);
        Context = *saved;
        return {};
    }

#endif
    for (;;) Arch::Halt();
    return Error(ENOSYS);
}

::Ref<Thread> Thread::Fork(Process* process)
{
#if CTOS_TARGET_X86_64
    auto newThread
        = process->CreateThread(Context.RIP, m_IsUser, CPU::GetCurrent()->ID);
    newThread->m_Tls.Self  = newThread.Raw();
    newThread->m_Tls.Stack = m_Tls.Stack;

    Pointer kstack = PMM::CallocatePages<uintptr_t>(CPU::KERNEL_STACK_SIZE
                                                    / PMM::PAGE_SIZE);
    newThread->m_Tls.KernelStack
        = kstack.ToHigherHalf<Pointer>().Offset<uintptr_t>(
            CPU::KERNEL_STACK_SIZE);

    Pointer pfstack = PMM::CallocatePages<uintptr_t>(CPU::KERNEL_STACK_SIZE
                                                     / PMM::PAGE_SIZE);
    newThread->m_Tls.PageFaultStack
        = pfstack.ToHigherHalf<Pointer>().Offset<uintptr_t>(
            CPU::KERNEL_STACK_SIZE);

    for (const auto& stack : m_Stacks)
    {
        usize stackVirt = stack->VirtualBase();

        auto  region    = process->m_AddressSpace.Find(stackVirt);
        if (!region) continue;
        newThread->m_Stacks.PushBack(region);
    }

    newThread->m_Tls.FpuStoragePageCount = m_Tls.FpuStoragePageCount;
    newThread->m_Tls.FpuStorage
        = Pointer(PMM::CallocatePages<uintptr_t>(m_Tls.FpuStoragePageCount))
              .ToHigherHalf<uintptr_t>();

    Memory::Copy(newThread->m_Tls.FpuStorage, m_Tls.FpuStorage,
                 m_Tls.FpuStoragePageCount * PMM::PAGE_SIZE);

    newThread->m_Parent    = process;
    newThread->Context     = SavedContext;
    newThread->Context.RAX = 0;
    newThread->Context.RDX = 0;

    newThread->m_IsUser    = m_IsUser;
    newThread->m_GsBase    = m_GsBase;
    newThread->m_FsBase    = m_FsBase;

    newThread->m_State     = ThreadState::eDequeued;
    return newThread;
#else
    return nullptr;
#endif
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
    // m_Parent->m_UserStackTop.Raw() - CPU::USER_STACK_SIZE;

    Assert(m_Parent->PageMap->MapRange(
        stackVirt, stackPhys, CPU::USER_STACK_SIZE,
        PageAttributes::eRWXU | PageAttributes::eWriteBack));

    using VMM::Access;
    auto stackRegion = new Region(stackPhys, stackVirt, CPU::USER_STACK_SIZE);
    stackRegion->SetAccessMode(Access::eReadWriteExecute | Access::eUser);
    m_Stacks.PushBack(stackRegion);
    m_Parent->m_AddressSpace.Insert(stackVirt, stackRegion);

    m_StackVirt              = stackVirt;
    m_Parent->m_UserStackTop = guardVirt.Raw() - PMM::PAGE_SIZE;
    return {stackPhys.ToHigherHalf<Pointer>().Offset(CPU::USER_STACK_SIZE),
            stackVirt.Offset(CPU::USER_STACK_SIZE)};
};
