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

        isize fdNum = OpenAt(AT_FDCWD, "/dev/console", O_RDWR, S_IFCHR).Value();

        DupFd(fdNum, 0, 0);
        DupFd(fdNum, 1, 0);
        DupFd(fdNum, 2, 0);
    }

    for (auto& timer : m_Timers) timer = CreateRef<struct Timer>();
}
// Frees the physical memory backing `region`, if the PMM owns it. Regions
// that were never touched (demand paging) have no backing memory, and device
// mappings (e.g. the framebuffer) point at memory the PMM never allocated.
static void FreeRegionMemory(const Ref<Region>& region)
{
    auto phys = region->PhysicalBase();
    if (!phys) return;

    auto fd   = region->FileDescriptor();
    auto file = fd ? fd->File() : nullptr;
    if (file && (file->IsCharDevice() || file->IsBlockDevice())) return;

    PMM::FreePages(phys, Math::DivRoundUp(region->Size(), PMM::PAGE_SIZE));
}

Process::~Process()
{
    if (PageMap)
    {
        Arch::VMM::DestroyPageMap(PageMap);
        delete PageMap;
        PageMap = nullptr;
    }
    if (!m_AddressSpace) return;

    for (auto& [virt, region] : *m_AddressSpace)
    {
        if (region->VirtualBase() == m_SignalTrampolineVirt)
        {
            LogDebug("~Process: Hit signal trampoline region, omitting...");
            continue;
        }
        FreeRegionMemory(region);
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
        Ref<::FileDescriptor> descriptor = GetFileHandle(dirFd);
        if (!descriptor) return Error(EBADF);
        auto entry = descriptor->DirectoryEntry();

        parent     = entry;
    }

    auto descriptor = TryOrRet(VFS::Open(parent, path, flags, mode));
    return m_FdTable->Insert(descriptor, -1, flags & O_CLOEXEC);
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
    if (newFdNum < 0) return Error(EBADF);

    Ref<::FileDescriptor> oldFd = GetFileHandle(oldFdNum);
    if (!oldFd) return Error(EBADF);

    // The new fd shares the open file description, but not FD_CLOEXEC
    newFdNum = m_FdTable->Replace(oldFd, newFdNum, flags & O_CLOEXEC);
    if (newFdNum < 0) return Error(EBADF);

    return newFdNum;
}
i32            Process::CloseFd(i32 fd) { return m_FdTable->Erase(fd); }
ErrorOr<isize> Process::InsertFd(Ref<::FileDescriptor> fd)
{
    return m_FdTable->Insert(fd, -1, fd->CloseOnExec());
}

ErrorOr<isize> Process::OpenPipe(i32* pipeFds, isize flags)
{
    auto pipe        = Fifo::CreatePipe();
    bool cloExec     = flags & O_CLOEXEC;
    i32  readerFdNum
        = static_cast<i32>(m_FdTable->Insert(pipe.Reader, -1, cloExec));
    CopyToUser(pipeFds, readerFdNum);
    i32 writerFdNum
        = static_cast<i32>(m_FdTable->Insert(pipe.Writer, -1, cloExec));
    CopyToUser(pipeFds + 1, writerFdNum);
    LogTrace("Process::OpenPipe: readerFd => {}, writerFd => {}", readerFdNum,
             writerFdNum);

    return 0;
}
ErrorOr<Ref<::FileDescriptor>> Process::GetFileDescriptor(isize fdNum)
{
    auto fd = m_FdTable->GetFd(fdNum);
    if (!fd) return Error(EBADF);

    return fd;
}
ErrorOr<Ref<::FileDescriptor>> Process::FileDescriptor(isize fdNum)
{
    return GetFileDescriptor(fdNum);
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
    auto pathRes = TryOrRet(VFS::ResolvePath(CWD().Raw(), path));
    auto dentry  = pathRes.Entry;
    auto inode   = dentry->INode();
    if (!inode) return Error(ENOENT);
    if (!inode->CanExecute(m_Credentials)) return Error(EPERM);

    // TODO(v1tr10l7): suid can be disable when mounting with flags
    if (inode->IsSetUserID()) m_Credentials.EffectiveUserID = inode->UserID();
    if (inode->IsSetGroupID())
        m_Credentials.EffectiveGroupID = inode->GroupID();

    isize size     = inode->Size();
    auto  buffer   = new u8[PMM::PAGE_SIZE];
    isize readSize = size < static_cast<isize>(PMM::PAGE_SIZE)
                       ? size
                       : static_cast<isize>(PMM::PAGE_SIZE);
    inode->Read(buffer, 0, readSize);
    StringView data(reinterpret_cast<char*>(buffer), readSize);
    bool       hasSheBang      = data.StartsWith("#!"_sv);

    StringView interpreterPath = "";
    if (hasSheBang)
    {
        auto spaceIt    = data.FindFirstOf(' ');
        auto count      = spaceIt != String::NPos ? spaceIt : data.Size();

        // FIXME(v1tr10l7): Validate path?
        interpreterPath = data.Substr(2, count - 2);
    }
    delete[] buffer;

    // Everything that can fail happens before the point of no return, so a
    // failed exec leaves the calling process intact. argv/envp live in the
    // old image, so they're copied into kernel memory before it goes away.
    if (hasSheBang && !interpreterPath.Empty())
    {
        auto pathRes = TryOrRet(VFS::ResolvePath(CWD().Raw(), interpreterPath));
        dentry       = pathRes.Entry;
        inode        = dentry->INode();
        if (!inode) return Error(ENOENT);
    }

    Vector<String> argvStorage;
    Vector<String> envpStorage;
    if (hasSheBang && !interpreterPath.Empty()) argvStorage.PushBack(path);
    {
        UserMemoryProtectionGuard guard;
        for (char** arg = argv; arg && *arg; arg++) argvStorage.PushBack(*arg);
        for (char** env = envp; env && *env; env++) envpStorage.PushBack(*env);
    }

    // Load the new image into a fresh page map and address space; the old
    // ones are still live (and loaded in CR3) at this point.
    auto newAddressSpace = CreateRef<class AddressSpace>();
    auto newPageMap      = new class PageMap();

    ExecutableProgram program;
    if (!program.Load(dentry, newPageMap, *newAddressSpace))
    {
        for (const auto& [virt, region] : *newAddressSpace)
            FreeRegionMemory(region);
        newAddressSpace->Clear();
        Arch::VMM::DestroyPageMap(newPageMap);
        delete newPageMap;
        return Error(ENOEXEC);
    }

    // Point of no return.
    auto oldTable = m_FdTable;
    m_FdTable     = CreateRef<class FileDescriptorTable>();

    if (oldTable)
        for (const auto& [i, fd] : *oldTable)
            if (!oldTable->IsCloseOnExec(i)) m_FdTable->Insert(fd, i);

    if (m_ID <= 2)
    {
        auto fdNumOr = OpenAt(AT_FDCWD, "/dev/console", O_RDWR, S_IFCHR);
        if (fdNumOr)
        {
            isize fdNum = fdNumOr.Value();
            DupFd(fdNum, 0, 0);
            DupFd(fdNum, 1, 0);
            DupFd(fdNum, 2, 0);
        }
        else LogError("Process::Exec: Failed to open /dev/console");
    }

    // Switch to the new page map *before* tearing down the old one: the old
    // page tables must not be freed while CR3 still points at them.
    auto oldPageMap     = PageMap;
    auto oldAddressSpace = m_AddressSpace;
    PageMap             = newPageMap;
    m_AddressSpace      = newAddressSpace;
    VMM::LoadPageMap(*PageMap, true);

    if (oldAddressSpace)
    {
        for (const auto& [virt, region] : *oldAddressSpace)
        {
            if (region->VirtualBase() == m_SignalTrampolineVirt) continue;

            FreeRegionMemory(region);
        }
        oldAddressSpace->Clear();
    }
    if (oldPageMap)
    {
        Arch::VMM::DestroyPageMap(oldPageMap);
        delete oldPageMap;
    }

    m_Name                = path;
    Thread* currentThread = CPU::GetCurrentThread();
    currentThread->SetState(ThreadState::eExited);

    // This thread is still executing (on its own kernel stack) until the
    // Yield() below, so keep a reference to it: CreateThread() replaces
    // m_MainThread, and dropping the last reference here would free the
    // kernel stack we're running on (seen as PMM double frees, then a crash).
    for (auto& thread : m_Threads)
        if (thread == currentThread) m_ExecutingThread = thread;
    m_Threads.Clear();

    Vector<StringView> argvArr;
    Vector<StringView> envpArr;
    for (const auto& arg : argvStorage) argvArr.PushBack(arg.View());
    for (const auto& env : envpStorage) envpArr.PushBack(env.View());

    auto thread
        = CreateThread(argvArr, envpArr, program, CPU::GetCurrent()->ID);

    Scheduler::EnqueueThread(thread.Raw());
    Scheduler::Yield();
    return 0;
}
ErrorOr<ProcessID> Process::WaitPid(ProcessID pid, i32* wstatus, i32 flags,
                                    rusage* rusage)
{
    bool block   = !(flags & WNOHANG);
    auto matches = [this, pid](Process* child) -> bool
    {
        if (pid < -1) return child->PGid() == -pid;
        if (pid == -1) return true;
        if (pid == 0) return child->PGid() == PGid();
        return child->ID() == pid;
    };
    // Reaps an exited child: it must not be reported (or waited on) again.
    auto reap = [this](Process* child)
    {
        for (auto it = m_Children.begin(); it != m_Children.end(); ++it)
        {
            if (*it != child) continue;
            m_Children.Erase(it);
            break;
        }
        child->MarkForTeardown(eReaped);
    };

    Vector<Event*>   events;
    Vector<Process*> procs;
    for (;;)
    {
        events.Clear();
        procs.Clear();

        // Consider *every* matching child, not just the first one.
        for (auto child : m_Children)
            if (matches(child)) procs.PushBack(child);
        if (procs.Empty()) return Error(ECHILD);

        // A child that exited before we got here has already triggered its
        // event; report it right away instead of relying on that event.
        for (auto child : procs)
        {
            if (child->m_State != ProcessState::eDead && !child->m_Exited)
                continue;

            // m_Status is already encoded with W_EXITCODE() by Exit().
            if (wstatus) CopyToUser(wstatus, child->m_Status.ValueOr(0));
            ProcessID foundId = child->ID();
            reap(child);
            return foundId;
        }

        // WNOHANG: children exist, but none has changed state.
        if (!block) return 0;

        for (auto child : procs) events.PushBack(&child->m_Event);
        auto ret = Event::Await(Span(events.Raw(), events.Size()), true);
        if (!ret.HasValue()) return Error(EINTR);

        // Don't trust which event woke us (a wakeup can arrive early): just
        // look at the children again. Exits are handled at the top of the
        // loop; here only stop/continue notifications are reported.
        if (ret.Value() >= procs.Size()) continue;
        auto which  = procs[ret.Value()];
        auto status = which->Status().ValueOr(0);
        if (which->m_Exited || which->m_State == ProcessState::eDead) continue;

        bool report = ((flags & WUNTRACED) && WIFSTOPPED(status))
                   || ((flags & WCONTINUED) && WIFCONTINUED(status));
        if (!report) continue;

        LogTrace("Process::WaitPid: Woken up for pid: {}", which->ID());
        if (wstatus) CopyToUser(wstatus, status);
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

void Process::MarkForTeardown(TeardownMark mark)
{
    u8 previous = m_TeardownMarks.FetchOr(mark);
    if (previous & mark) return;
    if ((previous | mark) == (eOffCpu | eReaped)) delete this;
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

    // Leave this page map before dropping it: CR3 must never point at a
    // freed page map. (Its tables are torn down by ~Process; with CLONE_VM
    // the map may be shared, so it isn't destroyed here.)
    VMM::LoadPageMap(*VMM::GetKernelPageMap(), false);
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
    for (auto& [fdNum, fd] : *m_FdTable)
        process->m_FdTable->Insert(fd, fdNum, m_FdTable->IsCloseOnExec(fdNum));
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
        auto  fd        = region->FileDescriptor();
        auto  file      = fd ? fd->File() : nullptr;
        bool  isDevice
            = file && (file->IsCharDevice() || file->IsBlockDevice());

        auto newRegion = process->m_AddressSpace->AllocateFixed(virt, size);
        if (!newRegion) return Error(ENOMEM);
        newRegion->SetAccessMode(region->Access());
        newRegion->SetFileDescriptor(fd, region->FileOffset());

        // Never touched yet: the child demand-pages it too. (Copying from
        // PhysicalBase() == nullptr would copy physical page 0 into it.)
        auto parentPhys = region->PhysicalBase();
        if (!parentPhys) continue;

        auto attributes = region->PageAttributes();
        if (virt < Pointer(MM::HigherHalfOffset()))
            attributes |= PageAttributes::eUser;

        // Device memory (e.g. a mapped framebuffer) is shared, not copied.
        if (isDevice)
        {
            newRegion->SetPhysicalBase(parentPhys);
            process->PageMap->MapRange(virt, parentPhys, size, attributes);
            continue;
        }

        auto phys = PMM::CallocatePages(pageCount);
        if (!phys) return Error(ENOMEM);
        Memory::Copy(Pointer(phys).ToHigherHalf<void*>(),
                     parentPhys.ToHigherHalf<void*>(), size);

        // Record the copy, so it is freed with the region and isn't treated
        // as unbacked by the page-fault handler.
        newRegion->SetPhysicalBase(phys);
        process->PageMap->MapRange(virt, phys, size, attributes);
    }

    return {};
}

void Process::ReapChild(Process* child)
{
    Assert(child->m_Parent == this);
    Assert(child->m_State == ProcessState::eZombie);
    child->m_State = ProcessState::eDead;

    auto it        = Find(m_Children.begin(), m_Children.end(), child);
    Assert(it != m_Children.end());
    m_Children.Erase(it);
    Scheduler::RemoveProcess(child->ID());
}
