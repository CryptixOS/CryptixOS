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
    : name(name)
    , ring(ring)
{
    nextTid = pid = AllocatePid();
    if (ring == PrivilegeLevel::ePrivileged) pageMap = VMM::GetKernelPageMap();
}

Process* Process::Fork()
{
    Thread* currentThread = CPU::GetCurrentThread();
    Assert(currentThread && currentThread->parent == this);

    Process* newProcess = new Process(name, ring);
    newProcess->parent  = this;

    PageMap* pageMap    = new PageMap();
    newProcess->pageMap = pageMap;

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
    }

    newProcess->nextTid.store(nextTid);
    for (usize i = 0; i < fileDescriptors.size(); i++)
    {
        FileDescriptor* currentFd = fileDescriptors[i];
        FileDescriptor* newFd     = currentFd->node->Open();

        newProcess->fileDescriptors.push_back(newFd);
    }

    m_Children.push_back(newProcess);
    newProcess->parent       = this;

    Thread* thread           = currentThread->Fork(newProcess);
    thread->enqueued         = false;
    newProcess->userStackTop = userStackTop;

    newProcess->threads.push_back(thread);
    Scheduler::EnqueueThread(thread);

    return newProcess;
}

i32 Process::Exit(i32 code)
{
    for (FileDescriptor* fd : fileDescriptors) fd->Close();
    fileDescriptors.clear();

    for (Thread* thread : threads) thread->state = ThreadState::eExited;

    Scheduler::Yield();
    CtosUnreachable();
}
