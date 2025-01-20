/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Scheduler/Process.hpp>

#include <Arch/CPU.hpp>

#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/Math.hpp>
#include <VFS/FileDescriptor.hpp>

inline usize AllocatePid()
{
    static std::atomic<pid_t> pid = 7;

    return pid++;
}

Process::Process(std::string_view name, PrivilegeLevel ring)
    : m_Name(name)
    , m_Ring(ring)
{
    m_NextTid = m_Pid = AllocatePid();
    if (ring == PrivilegeLevel::ePrivileged) PageMap = VMM::GetKernelPageMap();

    Scheduler::GetProcessList()[m_Pid] = this;
    m_CWD                              = VFS::GetRootNode();
}
Process::Process(std::string_view name, pid_t pid)
    : m_Pid(pid)
    , m_Name(name)
{
    Scheduler::GetProcessList()[m_Pid] = this;
    m_CWD                              = VFS::GetRootNode();
}

Process* Process::GetCurrent()
{
    Thread* currentThread = CPU::GetCurrentThread();
    return currentThread->parent;
}

void Process::InitializeStreams()
{
    m_FdTable.Clear();
    INode* currentTTY
        = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), "/dev/tty"));

    LogTrace("Process: Creating standard streams...");
    m_FdTable.Insert(new FileDescriptor(currentTTY, 0, FileAccessMode::eRead),
                     0);
    m_FdTable.Insert(new FileDescriptor(currentTTY, 0, FileAccessMode::eWrite),
                     1);
    m_FdTable.Insert(new FileDescriptor(currentTTY, 0, FileAccessMode::eWrite),
                     2);
}
bool Process::ValidateAddress(Pointer address, i32 accessMode)
{
    // TODO(v1tr10l7): Validate access mode
    for (const auto& region : m_AddressSpace)
        if (region.Contains(address)) return true;

    return false;
}

i32 Process::OpenAt(i32 dirFd, PathView path, i32 flags, mode_t mode)
{
    INode* parent = m_CWD;
    if (Path::IsAbsolute(path)) parent = VFS::GetRootNode();
    else if (dirFd != AT_FDCWD)
    {
        auto* descriptor = GetFileHandle(dirFd);
        if (!descriptor) return_err(-1, EBADF);
        parent = descriptor->GetNode();
    }

    auto descriptor = VFS::Open(parent, path, flags, mode);
    if (!descriptor) return -1;

    return m_FdTable.Insert(descriptor);
}
i32 Process::DupFd(i32 oldFdNum, i32 newFdNum, i32 flags)
{
    return_err(-1, ENOSYS);
}
i32 Process::CloseFd(i32 fd) { return m_FdTable.Erase(fd); }

i32 Process::Exec(const char* path, char** argv, char** envp)
{
    m_FdTable.Clear();
    InitializeStreams();

    m_Name = path;
    Arch::VMM::DestroyPageMap(PageMap);
    delete PageMap;
    PageMap = new class PageMap();

    static ELF::Image program, ld;
    Assert(program.Load(path, PageMap, m_AddressSpace));
    std::string_view ldPath = program.GetLdPath();
    if (!ldPath.empty())
    {
        LogTrace("Kernel: Loading ld: '{}'", ldPath);
        Assert(ld.Load(ldPath, PageMap, m_AddressSpace, 0x40000000));
    }
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
std::expected<pid_t, std::errno_t> Process::WaitPid(pid_t pid, i32* wstatus,
                                                    i32 flags, rusage* rusage)
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

    Process* newProcess    = new Process(m_Name, m_Ring);
    newProcess->m_Parent   = this;

    class PageMap* pageMap = new class PageMap();
    newProcess->PageMap    = pageMap;
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

    Scheduler::GetProcessList().erase(m_Pid);

    m_Status = code;

    Scheduler::Yield();
    CtosUnreachable();
}
