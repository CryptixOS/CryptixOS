/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/Singleton.hpp>
#include <Scheduler/Process.hpp>

class Process;
struct Thread;
class Scheduler
{
  public:
    KERNEL_INIT_CODE
    static void Initialize();
    KERNEL_INIT_CODE
    static void     InitializeProcFs();
    static void     PrepareAP(bool start = false);

    static bool     IsPreemptionEnabled();

    static void     EnablePreemption();
    static void     DisablePreemption();

    static void     Block(Thread* thread);
    static void     Unblock(Thread* thread);

    static void     Yield(bool saveCtx = false);

    static Process* KernelProcess();
    static Process* GetKernelProcess();

    static Process* CreateProcess(Process* parent, StringView name,
                                  const Credentials& creds);
    static void     RemoveProcess(ProcessID pid);

    static bool     ValidatePid(ProcessID pid);
    static Process* GetProcess(ProcessID pid);

    static void     EnqueueThread(Thread* thread);
    static void     EnqueueNotReady(Thread* thread);
    static void     DequeueThread(Thread* thread);

  private:
    Scheduler() = default;

    static Process::List& ProcessList();
    friend class Process;

    static Thread* GetNextThread(usize cpuID);
    static Thread* PickReadyThread();
    static void    SwitchContext(Thread*                  newThread,
                                 struct ExecutionContext* context);

    static void    Tick(struct ExecutionContext*);
}; // namespace Scheduler
