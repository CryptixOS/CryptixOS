
/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Thread.hpp"

#include "Arch/CPU.hpp"

#include "Memory/PMM.hpp"

#include "Scheduler/Process.hpp"
#include "Utility/Math.hpp"

static u64 AllocateTID()
{
    static usize i = 10;
    return i++;
}

Thread::Thread(Process* parent, uintptr_t pc, uintptr_t arg, i64 runOn)
    : runningOn(runOn)
    , self(this)
    , error(no_error)
    , parent(parent)
    , user(false)
    , enqueued(false)
    , state(ThreadState::eDequeued)

{
    tid = AllocateTID();
    CPU::PrepareThread(this, pc, arg);
    parent->threads.push_back(this);
}
Thread::Thread(Process* parent, uintptr_t pc, bool user)
    : runningOn(CPU::GetCurrent()->id)
    , self(this)
    , error(no_error)
    , parent(parent)
    , user(user)
    , enqueued(false)
    , state(ThreadState::eDequeued)
{
    tid = AllocateTID();

    uintptr_t pstack
        = PMM::CallocatePages<uintptr_t>(CPU::USER_STACK_SIZE / PMM::PAGE_SIZE);
    uintptr_t vustack = parent->userStackTop - CPU::USER_STACK_SIZE;

    if (!parent->pageMap) parent->pageMap = VMM::GetKernelPageMap();
    Assert(parent->pageMap->MapRange(vustack, pstack, CPU::USER_STACK_SIZE,
                                     PageAttributes::eRW | PageAttributes::eUser
                                         | PageAttributes::eWriteBack));
    parent->userStackTop = vustack - PMM::PAGE_SIZE;
    stacks.push_back(std::make_pair(pstack, CPU::USER_STACK_SIZE));

    [[maybe_unused]] uintptr_t stack1
        = ToHigherHalfAddress<uintptr_t>(pstack) + CPU::USER_STACK_SIZE;
    [[maybe_unused]] uintptr_t stack2 = vustack + CPU::USER_STACK_SIZE;

    this->stack                       = Math::AlignDown(stack1, 16);

    CPU::PrepareThread(this, pc, 0);
    parent->threads.push_back(this);
}
Thread::~Thread() {}
