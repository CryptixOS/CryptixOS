/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/sys/wait.h>
#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <Prism/Math.hpp>
#include <VFS/FileDescriptor.hpp>

#include <cctype>

inline usize AllocatePid()
{
    static Spinlock lock;
    ScopedLock      guard(lock);
    for (pid_t i = 1; i < std::numeric_limits<pid_t>::max(); i++)
        if (!Scheduler::ValidatePid(i)) return i;
    return -1;
}

Credentials Credentials::s_Root = {
    .uid  = 0,
    .gid  = 0,
    .euid = 0,
    .egid = 0,
    .suid = 0,
    .sgid = 0,
    .sid  = 0,
    .pgid = 0,
};

Process::Process(Process* parent, std::string_view name,
                 const Credentials& creds)
    : m_Parent(parent)
    , m_Pid(AllocatePid())
    , m_Name(name)
    , m_Credentials(creds)
    , m_Ring(PrivilegeLevel::eUnprivileged)
    , m_NextTid(m_Pid)

{
    m_CWD = VFS::GetRootNode();
    m_FdTable.OpenStdioStreams();
}

Process* Process::GetCurrent()
{
    Thread* currentThread = CPU::GetCurrentThread();

    return currentThread ? currentThread->m_Parent : nullptr;
}
Process* Process::CreateKernelProcess()
{
    Process* kernelProcess = Scheduler::GetKernelProcess();
    if (kernelProcess) goto ret;

    kernelProcess                = new Process;
    kernelProcess->m_Pid         = 0;
    kernelProcess->m_Name        = "TheOverlord";
    kernelProcess->PageMap       = VMM::GetKernelPageMap();
    kernelProcess->m_Credentials = Credentials::s_Root;
    kernelProcess->m_Ring        = PrivilegeLevel::ePrivileged;
    kernelProcess->m_NextTid     = 0;
    kernelProcess->m_Umask       = 0;

    // FIXME(v1tr10l7): What about m_AddressSpace?

ret:
    return kernelProcess;
}
Process* Process::CreateIdleProcess()
{
    static std::atomic<pid_t> idlePids(-1);

    std::string               name = "Idle Process for CPU: ";
    name += std::to_string(CPU::GetCurrentID());

    Process* idle = new Process;
    idle->m_Pid   = idlePids--;
    idle->m_Name  = name;
    idle->PageMap = VMM::GetKernelPageMap();

    return idle;
}

Thread* Process::CreateThread(uintptr_t rip, bool isUser, i64 runOn)
{
    auto thread      = new Thread(this, rip, 0, runOn);
    thread->m_IsUser = isUser;

    if (m_Threads.empty()) m_MainThread = thread;

    m_Threads.push_back(thread);
    return thread;
}
Thread* Process::CreateThread(uintptr_t                      rip,
                              std::vector<std::string_view>& argv,
                              std::vector<std::string_view>& envp,
                              ELF::Image& program, i64 runOn)
{
    auto thread = new Thread(this, rip, argv, envp, program, runOn);

    if (m_Threads.empty()) m_MainThread = thread;

    m_Threads.push_back(thread);
    return thread;
}

bool Process::ValidateAddress(Pointer address, i32 accessMode)
{
    // TODO(v1tr10l7): Validate access mode
    for (const auto& region : m_AddressSpace)
        if (region.Contains(address)) return true;

    return false;
}

pid_t Process::SetSid()
{
    m_Credentials.sid = m_Credentials.pgid = m_Pid;
    return m_Pid;
}

mode_t Process::Umask(mode_t mask)
{
    mode_t previous = m_Umask;
    m_Umask         = mask;

    return previous;
}
void Process::SendSignal(i32 signal)
{
    // TODO(v1tr10l7): implement signals
}

ErrorOr<i32> Process::OpenAt(i32 dirFd, PathView path, i32 flags, mode_t mode)
{
    INode* parent = m_CWD;
    if (path.IsAbsolute()) parent = VFS::GetRootNode();
    else if (dirFd != AT_FDCWD)
    {
        auto* descriptor = GetFileHandle(dirFd);
        if (!descriptor) return std::errno_t(EBADF);
        parent = descriptor->GetNode();
    }

    auto descriptor = VFS::Open(parent, path, flags, mode);
    if (!descriptor) return descriptor.error();

    return m_FdTable.Insert(descriptor.value());
}
ErrorOr<i32> Process::DupFd(i32 oldFdNum, i32 newFdNum, i32 flags)
{
    FileDescriptor* oldFd = GetFileHandle(oldFdNum);
    if (!oldFd) return std::errno_t(EBADF);

    FileDescriptor* newFd = GetFileHandle(newFdNum);
    if (newFd) CloseFd(newFdNum);

    newFd = new FileDescriptor(oldFd, flags);
    return m_FdTable.Insert(newFd, newFdNum);
}
i32          Process::CloseFd(i32 fd) { return m_FdTable.Erase(fd); }

ErrorOr<i32> Process::Exec(std::string path, char** argv, char** envp)
{
    m_FdTable.Clear();
    m_FdTable.OpenStdioStreams();

    m_Name = path;
    Arch::VMM::DestroyPageMap(PageMap);
    delete PageMap;
    PageMap = new class PageMap();

    std::vector<std::string_view> argvArr;
    static ELF::Image             program, ld;
    if (!program.Load(path, PageMap, m_AddressSpace)) return Error(ENOEXEC);

    std::string_view ldPath = program.GetLdPath();
    if (!ldPath.empty())
        Assert(ld.Load(ldPath, PageMap, m_AddressSpace, 0x40000000));
    Thread* currentThread = CPU::GetCurrentThread();
    currentThread->SetState(ThreadState::eExited);

    for (auto& thread : m_Threads)
    {
        if (thread == currentThread) continue;
        delete thread;
    }

    uintptr_t address
        = ldPath.empty() ? program.GetEntryPoint() : ld.GetEntryPoint();

    for (char** arg = argv; *arg; arg++) argvArr.push_back(*arg);

    std::vector<std::string_view> envpArr;
    for (char** env = envp; *env; env++) envpArr.push_back(*env);

    auto thread = CreateThread(address, argvArr, envpArr, program,
                               CPU::GetCurrent()->ID);
    Scheduler::EnqueueThread(thread);

    Scheduler::Yield();
    return 0;
}
ErrorOr<pid_t> Process::WaitPid(pid_t pid, i32* wstatus, i32 flags,
                                rusage* rusage)
{
    std::vector<Process*> procs;

    if (m_Children.empty()) return Error(ECHILD);
    for (auto& child : m_Children) procs.push_back(child);
    std::vector<Event*> events;
    for (auto& proc : procs) events.push_back(&proc->m_Event);

    for (;;)
    {
        auto ret = Event::Await(std::span(events.begin(), events.end()));
        if (!ret.has_value()) return Error(EINTR);

        auto which = procs[ret.value()];
        if (!(flags & WUNTRACED) && WIFSTOPPED(which->GetStatus().value_or(0)))
            continue;
        if (!(flags & WCONTINUED)
            && WIFCONTINUED(which->GetStatus().value_or(0)))
            continue;

        if (wstatus) *wstatus = W_EXITCODE(which->GetStatus().value_or(0), 0);

        return which->GetPid();
    }
}

ErrorOr<Process*> Process::Fork()
{
    Thread* currentThread = CPU::GetCurrentThread();
    Assert(currentThread && currentThread->m_Parent == this);

    Process* newProcess = Scheduler::CreateProcess(this, m_Name, m_Credentials);

    // TODO(v1tr10l7): implement PageMap::Fork;
    class PageMap* pageMap = new class PageMap();
    if (!pageMap) return Error(ENOMEM);

    newProcess->PageMap = pageMap;
    newProcess->m_CWD   = m_CWD;
    newProcess->m_Umask = m_Umask;
    m_Children.push_back(newProcess);

    for (auto& range : m_AddressSpace)
    {
        usize pageCount
            = Math::AlignUp(range.GetSize(), PMM::PAGE_SIZE) / PMM::PAGE_SIZE;

        uintptr_t physicalSpace = PMM::CallocatePages<uintptr_t>(pageCount);
        Assert(physicalSpace);

        std::memcpy(Pointer(physicalSpace).ToHigherHalf<void*>(),
                    range.GetPhysicalBase().ToHigherHalf<void*>(),
                    range.GetSize());
        pageMap->MapRange(range.GetVirtualBase(), physicalSpace,
                          range.GetSize(),
                          PageAttributes::eRWXU | PageAttributes::eWriteBack);
        newProcess->m_AddressSpace.push_back(
            {physicalSpace, range.GetVirtualBase(), range.GetSize()});

        // TODO(v1tr10l7): Free regions;
    }

    newProcess->m_NextTid.store(m_NextTid);
    for (const auto& [i, fd] : m_FdTable)
    {
        FileDescriptor* newFd = new FileDescriptor(fd);
        newProcess->m_FdTable.Insert(newFd, i);
    }

    Thread* thread             = currentThread->Fork(newProcess);
    thread->m_IsEnqueued       = false;
    newProcess->m_UserStackTop = m_UserStackTop;

    Scheduler::EnqueueThread(thread);

    return newProcess;
}

i32 Process::Exit(i32 code)
{
    AssertMsg(this != Scheduler::GetKernelProcess(),
              "Process::Exit(): The process with pid 1 tries to exit!");
    CPU::SetInterruptFlag(false);
    ScopedLock guard(m_Lock);

    // FIXME(v1tr10l7): Do proper cleanup of all resources
    m_FdTable.Clear();

    Thread* currentThread   = Thread::GetCurrent();
    currentThread->m_Parent = Scheduler::GetKernelProcess();

    Process* subreaper      = Scheduler::GetProcess(1);
    if (m_Pid > 1)
    {
        for (auto& child : m_Children)
        {
            child->m_Parent = subreaper;
            subreaper->m_Children.push_back(child);
        }
    }

    for (auto& zombie : m_Zombies)
    {
        zombie->m_Parent = m_Parent;
        m_Parent->m_Zombies.push_back(zombie);
    }
    m_Zombies.clear();

    delete PageMap;
    m_Status = W_EXITCODE(code, 0);
    m_Exited = true;

    // TODO(v1tr10l7): Free stacks

    for (Thread* thread : m_Threads)
    {
        // TODO(v1tr10l7): Wake up threads
        // auto state = thread->m_State;
        thread->SetState(ThreadState::eExited);

        // if (thread != currentThread && state != ThreadState::eRunning)
        //   CPU::WakeUp(thread->runningOn, false);
    }

    currentThread->SetState(ThreadState::eExited);
    Scheduler::RemoveProcess(m_Pid);
    VMM::LoadPageMap(*VMM::GetKernelPageMap(), false);

    Event::Trigger(&m_Event, false);

    Scheduler::Yield();
    AssertNotReached();
}
