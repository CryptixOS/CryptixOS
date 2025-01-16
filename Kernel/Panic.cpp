/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Arch/CPU.hpp>
#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

CTOS_NO_KASAN void logProcessState()
{
    Thread* currentThread = CPU::GetCurrentThread();
    if (!currentThread) return;
    Process* currentProcess = currentThread->parent;

    LogInfo("Process: {}", currentProcess->m_Name);
}
