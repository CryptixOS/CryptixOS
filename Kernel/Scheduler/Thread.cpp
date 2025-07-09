
/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/InterruptGuard.hpp>

#include <Memory/PMM.hpp>

#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

Thread::Thread(Process* parent, Pointer pc, Pointer arg, i64 runOn)
    : m_RunningOn(runOn)
    , m_Self(this)
    , m_State(ThreadState::eDequeued)
    , m_ErrorCode(no_error)
    , m_Parent(parent)
    , m_IsUser(false)
    , m_IsEnqueued(false)

{
    m_Tid = parent->m_NextTid++;
    CPU::PrepareThread(this, pc, arg);
}

Thread* Thread::Current() { return CPU::GetCurrentThread(); }

void    Thread::SetRunningOn(isize runningOn) { m_RunningOn = runningOn; }

Pointer Thread::GetStack() const { return m_Stack; }
void    Thread::SetStack(Pointer stack) { m_Stack = stack; }

Pointer Thread::PageFaultStack() const { return m_PageFaultStack; }
Pointer Thread::KernelStack() const { return m_KernelStack; }

void Thread::SetPageFaultStack(Pointer pfstack) { m_PageFaultStack = pfstack; }
void Thread::SetKernelStack(Pointer kstack) { m_KernelStack = kstack; }

Pointer Thread::FpuStorage() const { return m_FpuStorage; }
void    Thread::SetFpuStorage(Pointer fpuStorage, usize pageCount)
{
    m_FpuStorage          = fpuStorage;
    m_FpuStoragePageCount = pageCount;
}

static Pointer prepareStack(uintptr_t _stack, uintptr_t sp,
                            Vector<StringView> argv, Vector<StringView> envp,
                            ExecutableProgram& image)
{
    auto                           stack = reinterpret_cast<uintptr_t*>(_stack);

    CPU::UserMemoryProtectionGuard guard;
    for (auto env : envp)
    {
        stack = reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(stack)
                                             - env.Size() - 1);
        std::memcpy(stack, env.Raw(), env.Size());
    }

    for (auto arg : argv)
    {
        stack = reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(stack)
                                             - arg.Size() - 1);
        std::memcpy(stack, arg.Raw(), arg.Size());
    }

    stack = reinterpret_cast<uintptr_t*>(
        Math::AlignDown(reinterpret_cast<uintptr_t>(stack), 16));
    if ((argv.Size() + envp.Size() + 1) & 1) stack--;

    constexpr usize AT_ENTRY = 9;
    constexpr usize AT_PHDR  = 3;
    constexpr usize AT_PHENT = 4;
    constexpr usize AT_PHNUM = 5;

    *(--stack)               = 0;
    *(--stack)               = 0;
    stack -= 2;
    stack[0] = AT_ENTRY, stack[1] = image.Image().GetEntryPoint();
    stack -= 2;
    stack[0] = AT_PHDR, stack[1] = image.Image().GetAtPhdr();
    stack -= 2;
    stack[0] = AT_PHENT, stack[1] = image.Image().GetPhent();
    stack -= 2;
    stack[0] = AT_PHNUM, stack[1] = image.Image().GetPhNum();

    uintptr_t oldSp = sp;
    *(--stack)      = 0;
    stack -= envp.Size();
    for (usize i = 0; auto env : envp)
    {
        oldSp -= env.Size() + 1;
        stack[i++] = oldSp;
    }

    *(--stack) = 0;
    stack -= argv.Size();
    for (usize i = 0; auto arg : argv)
    {
        oldSp -= arg.Size() + 1;
        stack[i++] = oldSp;
    }

    *(--stack) = argv.Size();
    return sp - (_stack - reinterpret_cast<uintptr_t>(stack));
}

Thread::Thread(Process* parent, Pointer pc, Vector<StringView>& argv,
               Vector<StringView>& envp, ExecutableProgram& program, i64 runOn)
    : m_RunningOn(CPU::GetCurrent()->ID)
    , m_Self(this)
    , m_State(ThreadState::eDequeued)
    , m_ErrorCode(no_error)
    , m_Parent(parent)
    , m_IsUser(true)
    , m_IsEnqueued(false)
{
    m_Tid = parent->m_NextTid++;

    if (!parent->PageMap) parent->PageMap = VMM::GetKernelPageMap();

    auto mapUserStack = [this]() -> std::pair<uintptr_t, uintptr_t>
    {
        Pointer pstack
            = PMM::CallocatePages(CPU::USER_STACK_SIZE / PMM::PAGE_SIZE);
        Pointer vustack = m_Parent->m_UserStackTop.Raw() - CPU::USER_STACK_SIZE;

        Assert(m_Parent->PageMap->MapRange(
            vustack, pstack, CPU::USER_STACK_SIZE,
            PageAttributes::eRWXU | PageAttributes::eWriteBack));

        using VirtualMemoryManager::Access;
        auto stackRegion = new Region(pstack, vustack, CPU::USER_STACK_SIZE);
        stackRegion->SetProt(Access::eReadWriteExecute | Access::eUser,
                             PROT_READ | PROT_WRITE | PROT_EXEC);
        m_Stacks.PushBack(stackRegion);
        m_Parent->m_AddressSpace.Insert(vustack, stackRegion);

        m_StackVirt              = vustack;
        m_Parent->m_UserStackTop = vustack.Raw() - PMM::PAGE_SIZE;
        return {pstack.ToHigherHalf<Pointer>().Offset(CPU::USER_STACK_SIZE),
                vustack.Offset(CPU::USER_STACK_SIZE)};
    };

    auto [vstack, vustack] = mapUserStack();

    m_Stack                = prepareStack(vstack, vustack, argv, envp, program);
    CPU::PrepareThread(this, pc, 0);
}

Thread::Thread(Process* parent, Pointer pc, bool user)
    : m_RunningOn(CPU::GetCurrent()->ID)
    , m_Self(this)
    , m_State(ThreadState::eDequeued)
    , m_ErrorCode(no_error)
    , m_Parent(parent)
    , m_IsUser(user)
    , m_IsEnqueued(false)
{
    m_Tid = parent->m_NextTid++;

    Pointer pstack
        = PMM::CallocatePages<uintptr_t>(CPU::USER_STACK_SIZE / PMM::PAGE_SIZE);
    Pointer vustack = parent->m_UserStackTop.Raw() - CPU::USER_STACK_SIZE;

    if (!parent->PageMap) parent->PageMap = VMM::GetKernelPageMap();
    Assert(parent->PageMap->MapRange(vustack, pstack, CPU::USER_STACK_SIZE,
                                     PageAttributes::eRW | PageAttributes::eUser
                                         | PageAttributes::eWriteBack));
    parent->m_UserStackTop = vustack.Raw() - PMM::PAGE_SIZE;
    m_Stacks.PushBack(CreateRef<Region>(pstack, vustack, CPU::USER_STACK_SIZE));

    Pointer stack1
        = pstack.Offset<Pointer>(CPU::USER_STACK_SIZE).ToHigherHalf();

    this->m_Stack = Math::AlignDown(stack1, 16);

    CPU::PrepareThread(this, pc, 0);
}
Thread::~Thread() {}

void Thread::SendSignal(u8 signal)
{
    InterruptGuard guard(false);
    if (ShouldIgnoreSignal(signal)) return;
    m_PendingSignals |= 1 << signal;
}
bool Thread::DispatchAnyPendingSignal()
{
    Assert(!CPU::GetInterruptFlag());
    u32 pendingSignals = m_PendingSignals & ~m_SignalMask;

    u8  signal         = 0;
    for (; signal < 32; ++signal)
        if (pendingSignals & Bit(signal)) return DispatchSignal(signal);

    return false;
}
bool Thread::DispatchSignal(u8 signal)
{
    Assert(!CPU::GetInterruptFlag());
    Assert(signal < 32);

    m_PendingSignals &= ~Bit(signal);
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
            if (m_Parent->m_Pid != m_Parent->m_Credentials.pgid)
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
    return false;
}

Thread* Thread::Fork(Process* process)
{
    Thread* newThread
        = process->CreateThread(Context.rip, m_IsUser, CPU::GetCurrent()->ID);
    newThread->m_Self  = newThread;
    newThread->m_Stack = m_Stack;

    Pointer kstack     = PMM::CallocatePages<uintptr_t>(CPU::KERNEL_STACK_SIZE
                                                        / PMM::PAGE_SIZE);
    newThread->m_KernelStack = kstack.ToHigherHalf<Pointer>().Offset<uintptr_t>(
        CPU::KERNEL_STACK_SIZE);

    Pointer pfstack = PMM::CallocatePages<uintptr_t>(CPU::KERNEL_STACK_SIZE
                                                     / PMM::PAGE_SIZE);
    newThread->m_PageFaultStack
        = pfstack.ToHigherHalf<Pointer>().Offset<uintptr_t>(
            CPU::KERNEL_STACK_SIZE);

    for (const auto& stack : m_Stacks)
    {
        auto    stackPhys    = stack->PhysicalBase();
        usize   stackVirt    = stack->VirtualBase();
        usize   stackSize    = stack->Size();

        Pointer newStackPhys = PMM::CallocatePages<uintptr_t>(
            Math::AlignUp(stackSize, PMM::PAGE_SIZE) / PMM::PAGE_SIZE);

        std::memcpy(newStackPhys.ToHigherHalf<void*>(),
                    stackPhys.ToHigherHalf<void*>(), stackSize);

        process->PageMap->MapRange(stackVirt, newStackPhys, stackSize,
                                   PageAttributes::eRWXU
                                       | PageAttributes::eWriteBack);
        newThread->m_Stacks.PushBack(CreateRef<Region>(newStackPhys, newStackPhys, stackSize));
    }

    newThread->m_FpuStoragePageCount = m_FpuStoragePageCount;
    newThread->m_FpuStorage
        = Pointer(PMM::CallocatePages<uintptr_t>(m_FpuStoragePageCount))
              .ToHigherHalf<uintptr_t>();
    std::memcpy(newThread->m_FpuStorage, m_FpuStorage,
                m_FpuStoragePageCount * PMM::PAGE_SIZE);

    newThread->m_Parent    = process;
    newThread->Context     = SavedContext;
    newThread->Context.rax = 0;
    newThread->Context.rdx = 0;

    newThread->m_IsUser    = m_IsUser;
#ifdef CTOS_TARGET_X86_64
    newThread->m_GsBase = m_GsBase;
    newThread->m_FsBase = m_FsBase;
#endif

    newThread->m_State = ThreadState::eDequeued;
    return newThread;
}
