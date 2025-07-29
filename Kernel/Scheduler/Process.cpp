/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/sys/wait.h>
#include <Arch/CPU.hpp>

#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/Fifo.hpp>
#include <VFS/FileDescriptor.hpp>
#include <VFS/VFS.hpp>

inline usize AllocatePid()
{
    static Spinlock lock;
    ScopedLock      guard(lock);
    for (pid_t i = 1; i < std::numeric_limits<pid_t>::max(); i++)
        if (!Scheduler::ValidatePid(i)) return i;
    return -1;
}

Credentials Credentials::s_Root = {
    .UserID            = 0,
    .GroupID           = 0,
    .EffectiveUserID   = 0,
    .EffectiveGroupID  = 0,
    .FilesystemUserID  = 0,
    .FilesystemGroupID = 0,

    .SetUserID         = 0,
    .SetGroupID        = 0,
    .SessionID         = 0,
    .ProcessGroupID    = 0,
};

Process::Process(Process* parent, StringView name,
                 const struct Credentials& creds)
    : m_Parent(parent)
    , m_Pid(AllocatePid())
    , m_Name(name.Raw())
    , m_Credentials(creds)
    , m_Ring(PrivilegeLevel::eUnprivileged)
    , m_NextTid(m_Pid)
    , m_CWD(VFS::RootDirectoryEntry())

{
    m_FdTable.OpenStdioStreams();
}

Process* Process::GetCurrent()
{
    Thread* currentThread = CPU::GetCurrentThread();

    return currentThread ? currentThread->m_Parent : nullptr;
}
Process* Process::Current()
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
    kernelProcess->m_Name        = "TheOverlord"_s;
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
    static Atomic<pid_t> idlePids(-1);

    Process*             idle = new Process;

    idle->m_Pid               = idlePids--;
    idle->m_Name              = "Idle Process for CPU: "_s;
    idle->m_Name += StringUtils::ToString(CPU::GetCurrentID());

    idle->PageMap = VMM::GetKernelPageMap();

    return idle;
}

Thread* Process::CreateThread(Pointer rip, bool isUser, i64 runOn)
{
    auto thread      = new Thread(this, rip, 0, runOn);
    thread->m_IsUser = isUser;

    if (m_Threads.Empty()) m_MainThread = thread;

    m_Threads.PushBack(thread);
    return thread;
}
Thread* Process::CreateThread(Vector<StringView>& argv,
                              Vector<StringView>& envp,
                              ExecutableProgram& program, i64 runOn)
{
    auto thread = new Thread(this, argv, envp, program, runOn);

    if (m_Threads.Empty()) m_MainThread = thread;

    m_Threads.PushBack(thread);
    return thread;
}

bool Process::ValidateAddress(Pointer address, i32 accessMode, usize size)
{
    // TODO(v1tr10l7): Validate access mode
    for (const auto& [base, region] : m_AddressSpace)
    {
        if (region->Contains(address)) return true;
        if (!region->Contains(address)
            || !region->Contains(address.Offset(size)))
            continue;

        if (accessMode & PROT_READ && !region->IsReadable()) return false;
        if (accessMode & PROT_WRITE && !region->IsWriteable()) return false;
        if (accessMode & PROT_EXEC && !region->IsExecutable()) return false;
        return true;
    }
    return false;
}

pid_t Process::SetSid()
{
    m_Credentials.SessionID = m_Credentials.ProcessGroupID = m_Pid;
    return m_Pid;
}

ErrorOr<isize> Process::SetReUID(uid_t ruid, uid_t euid)
{
    m_Credentials.UserID          = ruid;
    m_Credentials.EffectiveUserID = euid;

    return {};
}
ErrorOr<isize> Process::SetReGID(gid_t rgid, gid_t egid)
{
    m_Credentials.GroupID          = rgid;
    m_Credentials.EffectiveGroupID = egid;

    return {};
}
ErrorOr<isize> Process::SetResUID(uid_t ruid, uid_t euid, uid_t suid)
{
    m_Credentials.UserID          = ruid;
    m_Credentials.EffectiveUserID = euid;
    m_Credentials.SetUserID       = suid;

    return {};
}
ErrorOr<isize> Process::SetResGID(gid_t rgid, gid_t egid, gid_t sgid)
{
    m_Credentials.GroupID          = rgid;
    m_Credentials.EffectiveGroupID = egid;
    m_Credentials.SetGroupID       = sgid;

    return {};
}

mode_t Process::Umask(mode_t mask)
{
    mode_t previous = m_Umask;
    m_Umask         = mask;

    return previous;
}

void Process::SendGroupSignal(pid_t pgid, i32 signal)
{
    auto& processMap = Scheduler::GetProcessMap();
    for (auto [pid, process] : processMap)
        if (process->Credentials().ProcessGroupID == pgid)
            process->SendSignal(signal);
}
void Process::SendSignal(i32 signal) { m_MainThread->SendSignal(signal); }

ErrorOr<isize> Process::OpenAt(i32 dirFd, PathView path, i32 flags, mode_t mode)
{
    Ref parent = CWD();
    if (CPU::AsUser([path]() -> bool { return path.Absolute(); }))
        parent = VFS::RootDirectoryEntry().Raw();
    else if (dirFd != AT_FDCWD)
    {
        Ref<FileDescriptor> descriptor = GetFileHandle(dirFd);
        if (!descriptor) return Error(EBADF);
        auto entry = descriptor->DirectoryEntry();

        parent     = entry;
    }

    auto descriptor
        = CPU::AsUser([&]() -> ErrorOr<Ref<FileDescriptor>>
                      { return VFS::Open(parent.Raw(), path, flags, mode); });
    if (!descriptor) return Error(descriptor.error());

    return m_FdTable.Insert(descriptor.value());
}
ErrorOr<isize> Process::DupFd(isize oldFdNum, isize newFdNum, isize flags)
{
    if (oldFdNum == newFdNum) return Error(EINVAL);

    Ref<FileDescriptor> oldFd = GetFileHandle(oldFdNum);
    if (!oldFd) return Error(EBADF);

    while (newFdNum < 0 || m_FdTable.IsValid(newFdNum)) ++newFdNum;
    Ref<FileDescriptor> newFd = GetFileHandle(newFdNum);
    if (newFd) CloseFd(newFdNum);

    // newFd = CreateRef<FileDescriptor>(oldFd, flags);
    // if (!newFd) return Error(ENOMEM);

    newFd    = oldFd;
    newFdNum = m_FdTable.Insert(newFd, newFdNum);
    if (newFdNum < 0) return Error(EBADF);

    return newFdNum;
}
i32            Process::CloseFd(i32 fd) { return m_FdTable.Erase(fd); }

ErrorOr<isize> Process::OpenPipe(i32* pipeFds)
{
    auto fifo     = new Fifo();
    auto readerFd = fifo->Open(Fifo::Direction::eRead);
    CPU::AsUser([&]() { pipeFds[0] = m_FdTable.Insert(readerFd); });

    auto writerFd = fifo->Open(Fifo::Direction::eWrite);
    CPU::AsUser([&]() { pipeFds[1] = m_FdTable.Insert(writerFd); });

    return 0;
}
ErrorOr<Ref<FileDescriptor>> Process::GetFileDescriptor(isize fdNum)
{
    auto fd = m_FdTable.GetFd(fdNum);
    if (!fd) return Error(EBADF);

    return fd;
}

Vector<String> SplitArguments(const String& str)
{
    Vector<String> segments;
    String         path(str.Raw(), str.Size());

    if (str.Empty()) return {""};
    usize start     = str[0] == ' ' ? 1 : 0;
    usize end       = start;

    auto  findSlash = [&str, &path](usize pos) -> usize
    {
        usize current = pos;
        while (current < path.Size() && str[current] != ' ') current++;

        return current == path.Size() ? String::NPos : current;
    };

    while ((end = findSlash(start)) < path.Size())
    {
        usize      segmentLength = end - start;
        StringView segment(path.Raw() + start, segmentLength);
        if (start != end) segments.PushBack(segment);

        start = end + 1;
    }

    // handle last segment
    if (start < path.Size())
        segments.EmplaceBack(path.Raw() + start, path.Size() - start);
    return segments;
}

ErrorOr<i32> Process::Exec(String path, char** argv, char** envp)
{
    m_FdTable.Clear();
    m_FdTable.OpenStdioStreams();

    for (const auto& [virt, region] : m_AddressSpace)
    {
        auto  phys      = region->PhysicalBase();
        usize pageCount = Math::DivRoundUp(region->Size(), PMM::PAGE_SIZE);
        PMM::FreePages(phys, pageCount);
    }

    m_Name = path;
    Arch::VMM::DestroyPageMap(PageMap);
    delete PageMap;

    PageMap = new class PageMap();

    Vector<StringView> argvArr;
    {
        CPU::UserMemoryProtectionGuard guard;
        // for (auto& arg : args) argvArr.EmplaceBack(arg.Raw(), arg.Size());
    }

    static ExecutableProgram program;

    if (!program.Load(path.Raw(), PageMap, m_AddressSpace))
        return Error(ENOEXEC);

    Thread* currentThread = CPU::GetCurrentThread();
    currentThread->SetState(ThreadState::eExited);

    for (auto& thread : m_Threads)
    {
        if (thread == currentThread) continue;
        delete thread;
    }

    {
        CPU::UserMemoryProtectionGuard guard;
        for (char** arg = argv; *arg; arg++) argvArr.PushBack(*arg);

        // for (usize i = 0; auto& arg : args)
        //     LogDebug("Process::Exec: argv[{}] = '{}'", i++, arg);
    }

    Vector<StringView> envpArr;
    {
        CPU::UserMemoryProtectionGuard guard;
        for (char** env = envp; *env; env++) envpArr.PushBack(*env);
    }

    auto thread
        = CreateThread(argvArr, envpArr, program, CPU::GetCurrent()->ID);
    Scheduler::EnqueueThread(thread);

    Scheduler::Yield();
    return 0;
}
ErrorOr<pid_t> Process::WaitPid(pid_t pid, i32* wstatus, i32 flags,
                                rusage* rusage)
{
    bool           block = !(flags & WNOHANG);
    Vector<Event*> events;
    for (;;)
    {
        events.Clear();
        events.ShrinkToFit();
        Process*         process = Process::GetCurrent();
        Vector<Process*> procs;
        if (m_Children.Empty()) return Error(ECHILD);

        if (pid < -1)
        {
            pid_t gid = -pid;
            auto  it  = std::find_if(m_Children.begin(), m_Children.end(),
                                     [gid](Process* proc) -> bool
                                     {
                                       if (proc->PGid() == gid) return true;
                                       return false;
                                   });
            if (it == m_Children.end()) return Error(ECHILD);
            procs.PushBack(*it);
        }
        else if (pid == -1) procs = process->m_Children;
        else if (pid == 0)
        {
            auto it = std::find_if(m_Children.begin(), m_Children.end(),
                                   [process](Process* proc) -> bool
                                   {
                                       if (proc->PGid() == process->PGid())
                                           return true;
                                       return false;
                                   });

            if (it == m_Children.end()) return Error(ECHILD);
            procs.PushBack(*it);
        }
        else if (pid > 0)
        {
            auto it = std::find_if(m_Children.begin(), m_Children.end(),
                                   [pid](Process* proc) -> bool
                                   {
                                       if (proc->Pid() == pid) return true;
                                       return false;
                                   });

            if (it == m_Children.end()) return Error(ECHILD);
            procs.PushBack(*it);
        }

        for (auto& proc : procs) events.PushBack(&proc->m_Event);

        auto ret = Event::Await(Span(events.Raw(), events.Size()), block);
        if (!ret.HasValue()) return Error(EINTR);

        auto which = procs[ret.Value()];
        if (!(flags & WUNTRACED) && WIFSTOPPED(which->Status().ValueOr(0)))
            continue;
        if (!(flags & WCONTINUED) && WIFCONTINUED(which->Status().ValueOr(0)))
            continue;

        if (wstatus)
            CPU::AsUser(
                [wstatus, &which]()
                { *wstatus = W_EXITCODE(which->Status().ValueOr(0), 0); });

        return which->Pid();
    }
}

ErrorOr<Process*> Process::Fork()
{
    LogDebug("Process: Forking {}...", m_Pid);
    Thread* currentThread = CPU::GetCurrentThread();
    Assert(currentThread && currentThread->m_Parent == this);

    Process* newProcess = Scheduler::CreateProcess(this, m_Name, m_Credentials);
    Assert(newProcess);

    LogTrace("Process: new process created!");
    // TODO(v1tr10l7): implement PageMap::Fork;
    class PageMap* pageMap = new class PageMap();
    if (!pageMap) return Error(ENOMEM);

    newProcess->PageMap = pageMap;
    newProcess->m_CWD   = m_CWD;
    newProcess->m_Umask = m_Umask;
    m_Children.PushBack(newProcess);

    newProcess->m_AddressSpace.Clear();
    LogDebug("Process: Copying the address space");
    for (const auto& [base, range] : m_AddressSpace)
    {
        usize pageCount
            = Math::AlignUp(range->Size(), PMM::PAGE_SIZE) / PMM::PAGE_SIZE;

        uintptr_t physicalSpace = PMM::CallocatePages<uintptr_t>(pageCount);
        Assert(physicalSpace);

        Memory::Copy(Pointer(physicalSpace).ToHigherHalf<void*>(),
                     range->PhysicalBase().ToHigherHalf<void*>(),
                     range->Size());
        pageMap->MapRange(range->VirtualBase(), physicalSpace, range->Size(),
                          PageAttributes::eRWXU | PageAttributes::eWriteBack);

        auto newRegion
            = new Region(physicalSpace, range->VirtualBase(), range->Size());
        newRegion->SetAccessMode(range->Access());
        newProcess->m_AddressSpace.Insert(range->VirtualBase().Raw(),
                                          newRegion);
        continue;
        // auto newRegion = newProcess->m_AddressSpace.AllocateFixed(
        //     range->VirtualBase(), range->Size());
        // if (!newRegion)
        //    newRegion
        //        = newProcess->m_AddressSpace.AllocateRegion(range->Size());
        newRegion->SetPhysicalBase(physicalSpace);
        newRegion->SetAccessMode(range->Access());

        // pageMap->MapRegion(newRegion);
        //  TODO(v1tr10l7): Free regions;
    }

    newProcess->m_NextTid.Store(m_NextTid.Load());
    LogDebug("Process: Copying fd table");
    for (const auto& [i, fd] : m_FdTable)
    {
        // Ref<FileDescriptor> newFd = new FileDescriptor(fd);
        newProcess->m_FdTable.Insert(fd, i);
    }

    Thread* thread             = currentThread->Fork(newProcess);
    thread->m_IsEnqueued       = false;
    newProcess->m_UserStackTop = m_UserStackTop;

    LogDebug("Process: enqueuing thread");
    Scheduler::EnqueueThread(thread);

    LogDebug("Process: Spawned {}", newProcess->m_Pid);
    return newProcess;
}

i32 Process::Exit(i32 code)
{
    LogDebug("Process: Exiting {} with exit code => {}", m_Pid, code);
    AssertMsg(this != Scheduler::GetKernelProcess(),
              "Process::Exit(): The process with pid 1 tries to exit!");
    CPU::SetInterruptFlag(false);
    ScopedLock guard(m_Lock);

    // FIXME(v1tr10l7): Do proper cleanup of all resources
    m_FdTable.Clear();

    Thread* currentThread   = Thread::Current();
    currentThread->m_Parent = Scheduler::GetKernelProcess();

    Process* subreaper      = Scheduler::GetProcess(1);
    if (m_Pid > 1)
    {
        for (auto& child : m_Children)
        {
            child->m_Parent = subreaper;
            subreaper->m_Children.PushBack(child);
        }
    }

    for (auto& zombie : m_Zombies)
    {
        zombie->m_Parent = m_Parent;
        m_Parent->m_Zombies.PushBack(zombie);
    }
    m_Zombies.Clear();

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

    LogDebug("Process: {} exited with exit code: {}", m_Pid, code);
    Scheduler::Yield();
    AssertNotReached();
}
