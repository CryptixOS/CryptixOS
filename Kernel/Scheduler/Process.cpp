/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/Math.hpp>
#include <VFS/FileDescriptor.hpp>

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

    return currentThread ? currentThread->parent : nullptr;
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
    kernelProcess->m_UMask       = 0;

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

ErrorOr<i32> Process::OpenAt(i32 dirFd, PathView path, i32 flags, mode_t mode)
{
    INode* parent = m_CWD;
    if (Path::IsAbsolute(path)) parent = VFS::GetRootNode();
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
i32 Process::CloseFd(i32 fd) { return m_FdTable.Erase(fd); }

i32 Process::Exec(const char* path, char** argv, char** envp)
{
    m_FdTable.Clear();
    m_FdTable.OpenStdioStreams();

    m_Name = path;
    Arch::VMM::DestroyPageMap(PageMap);
    delete PageMap;
    PageMap = new class PageMap();

    static ELF::Image program, ld;
    if (!program.Load(path, PageMap, m_AddressSpace)) return Error(ENOEXEC);

    std::string_view ldPath = program.GetLdPath();
    if (!ldPath.empty())
        Assert(ld.Load(ldPath, PageMap, m_AddressSpace, 0x40000000));
    Thread* currentThread = CPU::GetCurrentThread();
    currentThread->state  = ThreadState::eKilled;

    for (auto& thread : m_Threads)
    {
        if (thread == currentThread) continue;
        delete thread;
    }

    uintptr_t address
        = ldPath.empty() ? program.GetEntryPoint() : ld.GetEntryPoint();

    std::vector<std::string_view> argvArr;
    for (char** arg = argv; *arg; arg++) argvArr.push_back(*arg);

    std::vector<std::string_view> envpArr;
    for (char** env = envp; *env; env++) envpArr.push_back(*env);

    Scheduler::EnqueueThread(new Thread(this, address, argvArr, envpArr,
                                        program, CPU::GetCurrent()->ID));

    Scheduler::Yield();
    return 0;
}
ErrorOr<pid_t> Process::WaitPid(pid_t pid, i32* wstatus, i32 flags,
                                rusage* rusage)
{
    std::vector<Process*> procs;

    if (m_Children.empty()) return std::errno_t(ECHILD);
    for (auto& child : m_Children) procs.push_back(child);

    for (;;)
        for (const auto& child : procs)
        {
            if (!child->GetStatus()) continue;
            if (wstatus) *wstatus = child->GetStatus().value();
            return child->GetPid();
        }
}

Process* Process::Fork()
{
    Thread* currentThread = CPU::GetCurrentThread();
    Assert(currentThread && currentThread->parent == this);

    Process*       newProcess = new Process(this, m_Name, m_Credentials);

    class PageMap* pageMap    = new class PageMap();
    newProcess->PageMap       = pageMap;
    newProcess->m_CWD
        = m_CWD ? m_CWD->Reduce(false) : VFS::GetRootNode()->Reduce(false);

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
    for (const auto& file : m_FdTable)
    {
        FileDescriptor* fd = new FileDescriptor(file.second);
        newProcess->m_FdTable.Insert(fd, file.first);
    }

    m_Children.push_back(newProcess);
    newProcess->m_Parent       = this;

    Thread* thread             = currentThread->Fork(newProcess);
    thread->enqueued           = false;
    newProcess->m_UserStackTop = m_UserStackTop;

    newProcess->m_Threads.push_back(thread);
    Scheduler::EnqueueThread(thread);

    return newProcess;
}

i32 Process::Exit(i32 code)
{
    m_FdTable.Clear();
    for (Thread* thread : m_Threads) thread->state = ThreadState::eExited;

    Scheduler::RemoveProcess(m_Pid);

    m_Status = code;

    Scheduler::Yield();
    CtosUnreachable();
}
