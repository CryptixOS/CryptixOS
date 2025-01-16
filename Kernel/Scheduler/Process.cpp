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
}
Process::Process(std::string_view name, pid_t pid)
    : m_Pid(pid)
    , m_Name(name)
{
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
    Scheduler::EnqueueThread(
        new Thread(this, address, argv, envp, program, CPU::GetCurrent()->ID));

    Scheduler::Yield();
    return 0;
}
Process* Process::Fork()
{
    Thread* currentThread = CPU::GetCurrentThread();
    Assert(currentThread && currentThread->parent == this);

    Process* newProcess    = new Process(m_Name, m_Ring);
    newProcess->m_Parent   = this;

    class PageMap* pageMap = new class PageMap();
    newProcess->PageMap    = pageMap;

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
    for (const auto& descriptor : m_FdTable)
    {
        FileDescriptor* currentFd = descriptor.second;
        FileDescriptor* newFd     = currentFd->GetNode()->Open(0, 0);

        newProcess->m_FdTable[descriptor.first] = newFd;
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

    Scheduler::Yield();
    CtosUnreachable();
}
