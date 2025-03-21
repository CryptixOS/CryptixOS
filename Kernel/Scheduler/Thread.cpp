
/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Scheduler/Thread.hpp>

#include <Arch/CPU.hpp>
#include <Arch/InterruptGuard.hpp>

#include <Memory/PMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <Scheduler/Process.hpp>

Thread::Thread(Process* parent, uintptr_t pc, uintptr_t arg, i64 runOn)
    : runningOn(runOn)
    , self(this)
    , m_State(ThreadState::eDequeued)
    , m_ErrorCode(no_error)
    , m_Parent(parent)
    , m_IsUser(false)
    , m_IsEnqueued(false)

{
    m_Tid = parent->m_NextTid++;
    CPU::PrepareThread(this, pc, arg);
}

Thread*          Thread::GetCurrent() { return CPU::GetCurrentThread(); }

static uintptr_t prepareStack(uintptr_t _stack, uintptr_t sp,
                              std::vector<std::string_view> argv,
                              std::vector<std::string_view> envp,
                              ELF::Image&                   image)
{
    auto stack = reinterpret_cast<uintptr_t*>(_stack);

    for (auto env : envp)
    {
        stack = reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(stack)
                                             - env.length() - 1);
        std::memcpy(stack, env.data(), env.length());
    }

    for (auto arg : argv)
    {
        stack = reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(stack)
                                             - arg.length() - 1);
        std::memcpy(stack, arg.data(), arg.length());
    }

    stack = reinterpret_cast<uintptr_t*>(
        Math::AlignDown(reinterpret_cast<uintptr_t>(stack), 16));
    if ((argv.size() + envp.size() + 1) & 1) stack--;

    constexpr usize AT_ENTRY = 9;
    constexpr usize AT_PHDR  = 3;
    constexpr usize AT_PHENT = 4;
    constexpr usize AT_PHNUM = 5;

    *(--stack)               = 0;
    *(--stack)               = 0;
    stack -= 2;
    stack[0] = AT_ENTRY, stack[1] = image.GetEntryPoint();
    stack -= 2;
    stack[0] = AT_PHDR, stack[1] = image.GetAtPhdr();
    stack -= 2;
    stack[0] = AT_PHENT, stack[1] = image.GetPhent();
    stack -= 2;
    stack[0] = AT_PHNUM, stack[1] = image.GetPhNum();

    uintptr_t oldSp = sp;
    *(--stack)      = 0;
    stack -= envp.size();
    for (usize i = 0; auto env : envp)
    {
        oldSp -= env.length() + 1;
        stack[i++] = oldSp;
    }

    *(--stack) = 0;
    stack -= argv.size();
    for (usize i = 0; auto arg : argv)
    {
        oldSp -= arg.length() + 1;
        stack[i++] = oldSp;
    }

    *(--stack) = argv.size();
    return sp - (_stack - reinterpret_cast<uintptr_t>(stack));
}

Thread::Thread(Process* parent, uintptr_t pc,
               std::vector<std::string_view>& argv,
               std::vector<std::string_view>& envp, ELF::Image& program,
               i64 runOn)
    : runningOn(CPU::GetCurrent()->ID)
    , self(this)
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
        uintptr_t pstack  = PMM::CallocatePages<uintptr_t>(CPU::USER_STACK_SIZE
                                                          / PMM::PAGE_SIZE);
        uintptr_t vustack = m_Parent->m_UserStackTop - CPU::USER_STACK_SIZE;

        Assert(m_Parent->PageMap->MapRange(
            vustack, pstack, CPU::USER_STACK_SIZE,
            PageAttributes::eRWXU | PageAttributes::eWriteBack));
        m_Parent->m_AddressSpace.push_back(
            {pstack, vustack, CPU::USER_STACK_SIZE});
        m_StackVirt              = vustack;

        m_Parent->m_UserStackTop = vustack - PMM::PAGE_SIZE;

        m_Stacks.push_back(std::make_pair(pstack, CPU::USER_STACK_SIZE));
        return {ToHigherHalfAddress<uintptr_t>(pstack) + CPU::USER_STACK_SIZE,
                vustack + CPU::USER_STACK_SIZE};
    };

    auto [vstack, vustack] = mapUserStack();

    // argv.push_back("/usr/sbin/init");
    // envp.push_back("TERM=linux");

    this->stack            = prepareStack(vstack, vustack, argv, envp, program);

    CPU::PrepareThread(this, pc, 0);
}

Thread::Thread(Process* parent, uintptr_t pc, bool user)
    : runningOn(CPU::GetCurrent()->ID)
    , self(this)
    , m_State(ThreadState::eDequeued)
    , m_ErrorCode(no_error)
    , m_Parent(parent)
    , m_IsUser(user)
    , m_IsEnqueued(false)
{
    m_Tid = parent->m_NextTid++;

    uintptr_t pstack
        = PMM::CallocatePages<uintptr_t>(CPU::USER_STACK_SIZE / PMM::PAGE_SIZE);
    uintptr_t vustack = parent->m_UserStackTop - CPU::USER_STACK_SIZE;

    if (!parent->PageMap) parent->PageMap = VMM::GetKernelPageMap();
    Assert(parent->PageMap->MapRange(vustack, pstack, CPU::USER_STACK_SIZE,
                                     PageAttributes::eRW | PageAttributes::eUser
                                         | PageAttributes::eWriteBack));
    parent->m_UserStackTop = vustack - PMM::PAGE_SIZE;
    m_Stacks.push_back(std::make_pair(pstack, CPU::USER_STACK_SIZE));

    [[maybe_unused]] uintptr_t stack1
        = ToHigherHalfAddress<uintptr_t>(pstack) + CPU::USER_STACK_SIZE;
    [[maybe_unused]] uintptr_t stack2 = vustack + CPU::USER_STACK_SIZE;

    this->stack                       = Math::AlignDown(stack1, 16);

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
        = process->CreateThread(ctx.rip, m_IsUser, CPU::GetCurrent()->ID);
    newThread->self  = newThread;
    newThread->stack = stack;

    Pointer kstack   = PMM::CallocatePages<uintptr_t>(CPU::KERNEL_STACK_SIZE
                                                    / PMM::PAGE_SIZE);
    newThread->kernelStack = kstack.ToHigherHalf<Pointer>().Offset<uintptr_t>(
        CPU::KERNEL_STACK_SIZE);

    Pointer pfstack = PMM::CallocatePages<uintptr_t>(CPU::KERNEL_STACK_SIZE
                                                     / PMM::PAGE_SIZE);
    newThread->pageFaultStack
        = pfstack.ToHigherHalf<Pointer>().Offset<uintptr_t>(
            CPU::KERNEL_STACK_SIZE);

    for (const auto& [stackPhys, size] : m_Stacks)
    {
        uintptr_t newStack = PMM::CallocatePages<uintptr_t>(
            Math::AlignUp(size, PMM::PAGE_SIZE) / PMM::PAGE_SIZE);

        std::memcpy(Pointer(newStack).ToHigherHalf<void*>(),
                    Pointer(stackPhys).ToHigherHalf<void*>(), size);

        process->PageMap->MapRange(m_StackVirt, newStack, size,
                                   PageAttributes::eRWXU
                                       | PageAttributes::eWriteBack);
        newThread->m_Stacks.push_back({newStack, size});
    }

    newThread->fpuStoragePageCount = fpuStoragePageCount;
    newThread->fpuStorage
        = Pointer(PMM::CallocatePages<uintptr_t>(fpuStoragePageCount))
              .ToHigherHalf<uintptr_t>();
    std::memcpy((void*)newThread->fpuStorage, (void*)fpuStorage,
                fpuStoragePageCount * PMM::PAGE_SIZE);

    newThread->m_Parent = process;
    newThread->ctx      = SavedContext;
    newThread->ctx.rax  = 0;
    newThread->ctx.rdx  = 0;

    newThread->m_IsUser = m_IsUser;
#ifdef CTOS_TARGET_X86_64
    newThread->m_GsBase = m_GsBase;
    newThread->m_FsBase = m_FsBase;
#endif

    newThread->m_State = ThreadState::eDequeued;
    return newThread;
}
