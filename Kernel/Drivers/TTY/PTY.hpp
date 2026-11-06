/*
 * Created by v1tr10l7 on 05.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>
#include <Drivers/TTY/TTY.hpp>
#include <Library/Locking/Spinlock.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Scheduler/Event.hpp>

class MasterPTY;
class SlavePTY;

namespace PTY
{
    inline constexpr DeviceMajor MASTER_MAJOR  = 128;
    inline constexpr DeviceMajor SLAVE_MAJOR   = 136;
    inline constexpr usize       MAX_PTY_COUNT = 4096;
    inline constexpr usize       BUFFER_SIZE   = 0x1000;

    ErrorOr<usize>               AllocateIndex();
    void                         FreeIndex(usize index);
}; // namespace PTY

struct PTYPair : RefCounted<PTYPair>
{
    // Defined out of line: Ref<MasterPTY>/Ref<SlavePTY> need complete types
    explicit PTYPair(usize index);
    ~PTYPair();

    const usize             Index;
    ::Ref<MasterPTY>        Master;
    ::Ref<SlavePTY>         Slave;

    Spinlock                Lock;
    // slave -> master
    CircularQueue<u8, 4096> Output;
    // wakes master readers
    Event                   OutputReady;
    // wakes slave writers when full
    Event                   OutputSpace;

    bool                    Locked       = true;
    usize                   MasterOpens  = 0;
    usize                   SlaveOpens   = 0;
    bool                    MasterClosed = false;
    bool                    SlaveClosed  = false;
    bool                    TornDown     = false;

    usize                   PushOutput(const u8* src, usize count);
    usize                   PopOutput(u8* dest, usize count);

    void                    OnMasterClosed();
    void                    OnSlaveClosed();

  private:
    void TearDownIfUnused();
};
