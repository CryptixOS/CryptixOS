/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/linux/sched.h>
#include <API/Posix/sys/wait.h>
#include <Arch/CPU.hpp>

#include <Prism/Algorithm/Find.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/Fifo.hpp>
#include <VFS/FileDescriptor.hpp>
#include <VFS/VFS.hpp>

extern "C" char sigret_trampoline_start[];
extern "C" char sigret_trampoline_end[];

Pointer         g_SignalTrampoline = nullptr;
usize g_SignalTrampolineSize = sigret_trampoline_end - sigret_trampoline_start;

KeyValuePair<Pointer, usize> SignalTrampoline()
{
    static KeyValuePair signalTrampoline = []() -> KeyValuePair<Pointer, usize>
    {
        usize pageCount
            = Math::DivRoundUp(g_SignalTrampolineSize, PMM::PAGE_SIZE);

        LogTrace(
            "Process: Allocating the global signal trampoline => size: {:#x}, "
            "pageRoundedSize => {:#x}",
            g_SignalTrampolineSize, pageCount * PMM::PAGE_SIZE);
        g_SignalTrampoline = PMM::CallocatePages(pageCount);
        Memory::Copy(g_SignalTrampoline.ToHigherHalf(), sigret_trampoline_start,
                     g_SignalTrampolineSize);

        return {g_SignalTrampoline, g_SignalTrampolineSize};
    }();

    return signalTrampoline;
}

inline usize AllocatePid()
{
    static Spinlock lock;
    ScopedLock      guard(lock);
    for (ProcessID i = 1; i < std::numeric_limits<ProcessID>::max(); i++)
        if (!Scheduler::ValidatePid(i)) return i;
    return -1;
}

Process::Process(Process* parent, StringView name,
                 const struct Credentials& creds)
    : m_Parent(parent)
    , m_ID(AllocatePid())
    , m_Name(name)
    , m_Credentials(creds)
    , m_Ring(PrivilegeLevel::eUnprivileged)
    , m_NextTid(m_ID)

{
    if (m_ID == 1)
    {
        m_AddressSpace = CreateRef<class AddressSpace>();
        m_FsView       = CreateRef<FilesystemView>();
        m_FdTable      = CreateRef<class FileDescriptorTable>();
    }

    for (auto& timer : m_Timers) timer = CreateRef<struct Timer>();
}
Process::~Process()
{
    Arch::VMM::DestroyPageMap(PageMap);
    delete PageMap;
    if (!m_AddressSpace) return;

    for (auto& [virt, region] : *m_AddressSpace)
    {
        if (region->VirtualBase() == m_SignalTrampolineVirt)
        {
            LogDebug("~Process: Hit signal trampoline region, omitting...");
            continue;
        }
        auto  phys      = region->PhysicalBase();
        usize size      = region->Size();
        usize pageCount = Math::DivRoundUp(size, PMM::PAGE_SIZE);

        PMM::FreePages(phys, pageCount);
    }
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
    if (kernelProcess) return kernelProcess;

    kernelProcess                 = new Process;
    kernelProcess->m_ID           = 0;
    kernelProcess->m_Name         = "TheOverlord"_s;
    kernelProcess->PageMap        = VMM::GetKernelPageMap();
    kernelProcess->m_Credentials  = s_RootCredentials;
    kernelProcess->m_Ring         = PrivilegeLevel::ePrivileged;
    kernelProcess->m_NextTid      = 0;
    kernelProcess->m_FsView       = CreateRef<FilesystemView>();
    kernelProcess->m_FdTable      = CreateRef<class FileDescriptorTable>();
    kernelProcess->m_AddressSpace = CreateRef<class AddressSpace>();

    // FIXME(v1tr10l7): What about m_AddressSpace?
    return kernelProcess;
}
Process* Process::CreateIdleProcess()
{
    static Atomic<ProcessID> idlePids(-1);

    Process*                 idle = new Process;

    idle->m_ID                    = idlePids--;
    idle->m_Name                  = "Idle Process for CPU: "_s;
    idle->m_Name += StringUtils::ToString(CPU::GetCurrentID());
    idle->PageMap = VMM::GetKernelPageMap();

    return idle;
}

void Process::ForEach(Iterator it)
{
    for (auto& process : Scheduler::ProcessList())
        if (it(&process) == IterationResult::eBreak) break;
}
void Process::ForEachInGroup(ProcessID pgid, Iterator it)
{
    for (auto& process : Scheduler::ProcessList())
    {
        if (process.Credentials().ProcessGroupID != pgid) continue;

        if (it(&process) == IterationResult::eBreak) break;
    }
}

Ref<Thread> Process::CreateThread(Pointer rip, bool isUser, i64 runOn)
{
    auto thread = CreateRef<Thread>(this, rip, nullptr, runOn, isUser);

    if (m_Threads.Empty()) m_MainThread = thread;
    m_Threads.PushBack(thread);
    return thread;
}
Ref<Thread> Process::CreateThread(Vector<StringView>& argv,
                                  Vector<StringView>& envp,
                                  ExecutableProgram& program, i64 runOn)
{
    auto thread = CreateRef<Thread>(this, argv, envp, program, runOn);

    if (m_Threads.Empty()) m_MainThread = thread;
    m_Threads.PushBack(thread);
    return thread;
}

bool Process::ValidateAddress(Pointer address, i32 accessMode, usize size)
{
    // TODO(v1tr10l7): Validate access mode
    if (!m_AddressSpace) return false;
    for (const auto& [base, region] : *m_AddressSpace)
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

ProcessID Process::SetSid()
{
    m_Credentials.SessionID = m_Credentials.ProcessGroupID = m_ID;
    return m_ID;
}

ErrorOr<void> Process::SetUserID(::UserID uid)
{
    m_Credentials.UserID = uid;
    return {};
}
ErrorOr<void> Process::SetGroupID(::GroupID gid)
{
    m_Credentials.GroupID = gid;
    return {};
}

ErrorOr<isize> Process::SetReUID(::UserID ruid, ::UserID euid)
{
    if (ruid != static_cast<::UserID>(-1)) m_Credentials.UserID = ruid;
    if (euid != static_cast<::UserID>(-1)) m_Credentials.EffectiveUserID = euid;

    return {};
}
ErrorOr<isize> Process::SetReGID(::GroupID rgid, ::GroupID egid)
{
    if (rgid != static_cast<::GroupID>(-1)) m_Credentials.GroupID = rgid;
    if (egid != static_cast<::GroupID>(-1))
        m_Credentials.EffectiveGroupID = egid;

    return {};
}
ErrorOr<isize> Process::SetResUID(::UserID ruid, ::UserID euid, ::UserID suid)
{
    if (ruid != static_cast<::UserID>(-1)) m_Credentials.UserID = ruid;
    if (euid != static_cast<::UserID>(-1)) m_Credentials.EffectiveUserID = euid;
    if (suid != static_cast<::UserID>(-1)) m_Credentials.SetUserID = suid;

    return {};
}
ErrorOr<isize> Process::SetResGID(::GroupID rgid, ::GroupID egid,
                                  ::GroupID sgid)
{
    if (rgid != static_cast<::GroupID>(-1)) m_Credentials.GroupID = rgid;
    if (egid != static_cast<::GroupID>(-1))
        m_Credentials.EffectiveGroupID = egid;
    if (sgid != static_cast<::GroupID>(-1)) m_Credentials.SetGroupID = sgid;

    return {};
}

INodeMode Process::Umask(INodeMode mask)
{
    INodeMode previous = m_FsView->FileCreationMask();
    m_FsView->SetFileCreationMask(mask);

    return previous;
}

const struct SignalAction& Process::SignalAction(SignalID signal) const
{
    Assert(signal <= SignalID::eLastRealTime);

    const struct SignalAction* action;
    m_SignalActions.With([&action, signal](auto& actions)
                         { action = &actions[ToUnderlying(signal)]; });
    return *action;
}
void Process::SetSignalAction(SignalID                   signal,
                              const struct SignalAction& action)
{
    Assert(signal <= SignalID::eLastRealTime);

    m_SignalActions.With([&action, signal](auto& actions)
                         { actions[ToUnderlying(signal)] = action; });
}

void Process::SendGroupSignal(ProcessID pgid, i32 signal)
{
    for (auto& process : Scheduler::ProcessList())
        if (process.Credentials().ProcessGroupID == pgid)
            process.SendSignal(signal);
}
void Process::SendSignal(i32 signal) { m_MainThread->SendSignal(signal); }

ErrorOr<isize> Process::OpenAt(i32 dirFd, PathView path, i32 flags,
                               INodeMode mode)
{
    Ref parent = CWD();
    if (path.Absolute()) parent = VFS::RootDirectoryEntry();
    else if (dirFd != AT_FDCWD)
    {
        Ref<FileDescriptor> descriptor = GetFileHandle(dirFd);
        if (!descriptor) return Error(EBADF);
        auto entry = descriptor->DirectoryEntry();

        parent     = entry;
    }

    auto descriptor = TryOrRet(VFS::Open(parent, path, flags, mode));
    return m_FdTable->Insert(descriptor);
}
isize Process::FirstFreeFdIndex()
{
    isize fdNum = 0;
    while (m_FdTable->IsValid(fdNum)) ++fdNum;

    return fdNum;
}
ErrorOr<isize> Process::DupFd(isize oldFdNum, isize newFdNum, isize flags)
{
    if (oldFdNum == newFdNum) return Error(EINVAL);

    Ref<FileDescriptor> oldFd = GetFileHandle(oldFdNum);
    if (!oldFd) return Error(EBADF);

    Ref<FileDescriptor> newFd = GetFileHandle(newFdNum);
    if (newFd) CloseFd(newFdNum);

    newFd    = oldFd;
    newFdNum = m_FdTable->Insert(newFd, newFdNum);
    if (newFdNum < 0) return Error(EBADF);

    return newFdNum;
}
i32            Process::CloseFd(i32 fd) { return m_FdTable->Erase(fd); }
ErrorOr<isize> Process::InsertFd(Ref<FileDescriptor> fd)
{
    return m_FdTable->Insert(fd);
}

ErrorOr<isize> Process::OpenPipe(i32* pipeFds)
{
    auto fifo        = CreateRef<Fifo>();

    auto readerFd    = fifo->OpenDirection(Fifo::Direction::eRead);
    i32  readerFdNum = static_cast<i32>(m_FdTable->Insert(readerFd));
    CopyToUser(pipeFds, readerFdNum);
    auto writerFd    = fifo->OpenDirection(Fifo::Direction::eWrite);
    i32  writerFdNum = static_cast<i32>(m_FdTable->Insert(writerFd));
    CopyToUser(pipeFds + 1, writerFdNum);
    LogTrace("Process::OpenPipe: readerFd => {}, writerFd => {}", readerFdNum,
             writerFdNum);

    return 0;
}
ErrorOr<Ref<FileDescriptor>> Process::GetFileDescriptor(isize fdNum)
{
    auto fd = m_FdTable->GetFd(fdNum);
    if (!fd) return Error(EBADF);

    return fd;
}

Vector<String> SplitArguments(const String& str)
{
    Vector<String> segments;
    String         path = str;

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
        StringView segment       = path.Substr(start, segmentLength);
        if (start != end) segments.PushBack(segment);

        start = end + 1;
    }

    // handle last segment
    if (start < path.Size())
        segments.EmplaceBack(path.Substr(start, path.Size() - start));
    return segments;
}

ErrorOr<i32> Process::Exec(String path, char** argv, char** envp)
{
    if (m_FdTable) m_FdTable->Clear();
    m_FdTable = CreateRef<class FileDescriptorTable>();

    auto tty  = VFS::Open(VFS::RootDirectoryEntry(), "/dev/console", O_RDWR, 0)
                   .Value();
    m_FdTable->Insert(tty, 0);
    m_FdTable->Insert(tty, 1);
    m_FdTable->Insert(tty, 2);

    if (m_AddressSpace)
    {
        for (const auto& [virt, region] : *m_AddressSpace)
        {
            if (region->VirtualBase() == m_SignalTrampolineVirt) continue;

            auto  phys      = region->PhysicalBase();
            usize pageCount = Math::DivRoundUp(region->Size(), PMM::PAGE_SIZE);
            PMM::FreePages(phys, pageCount);
        }
        m_AddressSpace->Clear();
    }
    m_AddressSpace = CreateRef<class AddressSpace>();

    m_Name         = path;
    Arch::VMM::DestroyPageMap(PageMap);
    delete PageMap;

    PageMap = new class PageMap();
    Vector<StringView> argvArr;
    {
        UserMemoryProtectionGuard guard;
        for (char** arg = argv; *arg; arg++) argvArr.PushBack(*arg);
    }

    ExecutableProgram program;
    if (!program.Load(path, PageMap, *m_AddressSpace)) return Error(ENOEXEC);
    Thread* currentThread = CPU::GetCurrentThread();
    currentThread->SetState(ThreadState::eExited);

    for (auto& thread : m_Threads)
        // NOTE(v1tr10l7): We don't won't this thread to be deleted just yet, as
        // it is being executed right now. The scheduler will take care of
        // finalizing dead processes,
        // and then it will get cleanup up
        if (thread == currentThread) m_MainThread = currentThread;
    m_Threads.Clear();

    Vector<StringView> envpArr;
    {
        UserMemoryProtectionGuard guard;
        for (char** env = envp; *env; env++) envpArr.PushBack(*env);
    }

    auto thread
        = CreateThread(argvArr, envpArr, program, CPU::GetCurrent()->ID);

    Scheduler::EnqueueThread(thread.Raw());
    Scheduler::Yield();
    return 0;
}
ErrorOr<ProcessID> Process::WaitPid(ProcessID pid, i32* wstatus, i32 flags,
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
            ProcessID gid = -pid;
            auto      it  = FindIf(m_Children.begin(), m_Children.end(),
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
            auto it = FindIf(m_Children.begin(), m_Children.end(),
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
            auto it = FindIf(m_Children.begin(), m_Children.end(),
                             [pid](Process* proc) -> bool
                             {
                                 if (proc->ID() == pid) return true;
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
            CopyToUser(wstatus, W_EXITCODE(which->Status().ValueOr(0), 0));

        return which->ID();
    }
}

ErrorOr<Process*> Process::Clone(usize flags)
{
    LogDebug("Process: Forking {}...", m_ID);
    Assert(Thread::Current() && Thread::Current()->m_Parent == this);

    CPU::DisableInterrupts();
    Process* newProcess = Scheduler::CreateProcess(this, m_Name, m_Credentials);
    Assert(newProcess);

    // TODO(v1tr10l7): implement PageMap::Fork;
    newProcess->PageMap        = PageMap;
    newProcess->m_AddressSpace = m_AddressSpace;

    if (!(flags & CLONE_VM))
    {
        auto status = CopyMemory(newProcess);
        if (!status) return Error(status.Error());
    }
    newProcess->m_Parent->m_Children.PushBack(newProcess);
    newProcess->m_NextTid.Store(m_NextTid.Load());
    newProcess->m_FsView = m_FsView;
    if (!(flags & CLONE_FS)) CopyFs(newProcess);

    newProcess->m_FdTable = m_FdTable;
    if (!(flags & CLONE_FILES)) CopyFileDescriptors(newProcess);
    if (flags & CLONE_PARENT) newProcess->m_Parent = m_Parent;

    newProcess->m_UserStackTop = m_UserStackTop;
    LogDebug("Process: Spawned {}", newProcess->m_ID);
    return newProcess;
}

i32 Process::Exit(i32 code)
{
    AssertMsg(this != Scheduler::GetKernelProcess(),
              "Process::Exit(): The process with pid 1 tries to exit!");
    Assert(m_ID != 1 && "Process: init process tries to exit");
    CPU::SetInterruptFlag(false);
    ScopedLock guard(m_Lock);

    // FIXME(v1tr10l7): Do proper cleanup of all resources
    m_FdTable->Clear();

    Thread* currentThread   = Thread::Current();
    currentThread->m_Parent = Scheduler::GetKernelProcess();

    Process* subreaper      = Scheduler::GetProcess(1);
    if (m_ID > 1)
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

    for (auto thread : m_Threads)
    {
        // TODO(v1tr10l7): Wake up threads
        // auto state = thread->m_State;
        thread->SetState(ThreadState::eExited);
        m_MainThread = thread;

        // if (thread != currentThread && state != ThreadState::eRunning)
        //   CPU::WakeUp(thread->runningOn, false);
    }
    m_Threads.Clear();

    currentThread->SetState(ThreadState::eExited);
    m_State = ProcessState::eDead;

    Scheduler::RemoveProcess(m_ID);
    VMM::LoadPageMap(*VMM::GetKernelPageMap(), false);

    Event::Trigger(&m_Event, false);

    LogDebug("Process: {} exited with exit code: {}", m_ID, code);
    Scheduler::Yield();
    AssertNotReached();
}

ErrorOr<void> Process::WaitForFutex(i32* vaddr, i32 expected)
{
    {
        UserMemoryProtectionGuard guard;
        if (*vaddr != expected) return Error(EAGAIN);
    }

    auto it = m_FutexEvents.Find(reinterpret_cast<upointer>(vaddr));
    if (it == m_FutexEvents.end())
        m_FutexEvents[reinterpret_cast<upointer>(vaddr)] = new Event;
    auto event = m_FutexEvents[reinterpret_cast<upointer>(vaddr)] = new Event;

    bool interrupted = !event->Await(true);
    if (interrupted) return Error(EINTR);

    return {};
}
ErrorOr<void> Process::WakeFutex(i32* vaddr)
{
    {
        UserMemoryProtectionGuard guard;
        *(volatile int*)vaddr;
    }

    auto it = m_FutexEvents.Find(reinterpret_cast<upointer>(vaddr));
    if (it == m_FutexEvents.end())
        m_FutexEvents[reinterpret_cast<upointer>(vaddr)] = new Event;
    auto event = m_FutexEvents[reinterpret_cast<upointer>(vaddr)] = new Event;

    event->Trigger(false);
    return {};
}

void Process::CopyFs(Process* process)
{
    process->m_FsView = CreateRef<FilesystemView>(*m_FsView);

    m_FsView->SetRoot(m_FsView->Root());
    m_FsView->ChangeDirectory(m_FsView->WorkingDirectory());
    m_FsView->SetFileCreationMask(m_FsView->FileCreationMask());
}
void Process::CopyFileDescriptors(Process* process)
{
    LogTrace("Copying file descriptors");
    process->m_FdTable = CreateRef<class FileDescriptorTable>();
    for (auto& [fdNum, fd] : *m_FdTable) process->m_FdTable->Insert(fd, fdNum);
}
ErrorOr<void> Process::CopyMemory(Process* process)
{
    class PageMap* pageMap = new class PageMap();
    if (!pageMap) return Error(ENOMEM);

    process->PageMap        = pageMap;
    process->m_AddressSpace = CreateRef<class AddressSpace>();

    for (auto& [virt, region] : *m_AddressSpace)
    {
        if (region->VirtualBase() == m_SignalTrampolineVirt) continue;
        usize size      = region->Size();
        usize pageCount = Math::DivRoundUp(size, PMM::PAGE_SIZE);
        auto  phys      = PMM::CallocatePages(pageCount);

        Memory::Copy(Pointer(phys).ToHigherHalf<void*>(),
                     region->PhysicalBase().ToHigherHalf<void*>(),
                     region->Size());
        auto newRegion = process->m_AddressSpace->AllocateFixed(virt, size);
        newRegion->SetAccessMode(region->Access());
        process->PageMap->MapRange(virt, phys, size, region->PageAttributes());
    }

    return {};
}
