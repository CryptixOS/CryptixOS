/*
 * Created by v1tr10l7 on 05.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/TTY/MasterPTY.hpp>
#include <Drivers/TTY/PTY.hpp>
#include <Drivers/TTY/SlavePTY.hpp>
#include <VFS/DevPtsFs/DevPtsFs.hpp>

namespace PTY
{
    static Spinlock s_IndexLock;
    static Bitmap   s_Indices;
    static bool     s_IndicesReady = false;

    ErrorOr<usize>  AllocateIndex()
    {
        ScopedLock guard(s_IndexLock);
        if (!s_IndicesReady)
        {
            s_Indices.Allocate(MAX_PTY_COUNT);
            s_IndicesReady = true;
        }

        isize index = s_Indices.FindFirstNotSet(0, MAX_PTY_COUNT);
        if (index < 0) return Error(ENOSPC);

        s_Indices.SetIndex(index, true);
        return static_cast<usize>(index);
    }

    void FreeIndex(usize index)
    {
        ScopedLock guard(s_IndexLock);
        if (index < MAX_PTY_COUNT) s_Indices.SetIndex(index, false);
    }
}; // namespace PTY

PTYPair::PTYPair(usize index)
    : Index(index)
{
}
PTYPair::~PTYPair() = default;

usize PTYPair::PushOutput(const u8* src, usize count)
{
    ScopedLock guard(Lock);
    usize      pushed = 0;

    while (pushed < count && Output.Size() < PTY::BUFFER_SIZE)
        Output.Push(u8(src[pushed++]));
    if (pushed) OutputReady.Trigger();

    return pushed;
}
usize PTYPair::PopOutput(u8* dest, usize count)
{
    ScopedLock guard(Lock);
    usize      popped = 0;

    while (popped < count && !Output.Empty()) dest[popped++] = Output.Pop();
    if (popped) OutputSpace.Trigger();

    return popped;
}

void PTYPair::OnMasterClosed()
{
    {
        ScopedLock guard(Lock);
        MasterClosed = true;
    }

    if (Slave) Slave->Hangup();
    OutputSpace.Trigger();
    TearDownIfUnused();
}
void PTYPair::OnSlaveClosed()
{
    {
        ScopedLock guard(Lock);
        SlaveClosed = true;
    }

    // Wake master readers, so they see the hang-up
    OutputReady.Trigger();
    TearDownIfUnused();
}

void PTYPair::TearDownIfUnused()
{
    ::Ref<PTYPair> keepAlive = this;
    {
        ScopedLock guard(Lock);
        if (TornDown || MasterOpens || SlaveOpens) return;
        TornDown = true;
    }

    if (auto devpts = DevPtsFs::Instance()) devpts->RemoveSlave(Index);
    if (Slave) DeviceManager::UnregisterCharDevice(Slave.Raw());
    if (Master) DeviceManager::UnregisterCharDevice(Master.Raw());
    PTY::FreeIndex(Index);

    Master = nullptr;
    Slave  = nullptr;
}
