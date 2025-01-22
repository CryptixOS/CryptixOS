/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/KernelHeap.hpp>

#include <Scheduler/Process.hpp>
#include <Utility/Math.hpp>

#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>

TmpFsINode::TmpFsINode(INode* parent, std::string_view name, Filesystem* fs,
                       mode_t mode)
    : INode(parent, name, fs)
{
    Process* process   = Process::GetCurrent();

    m_Stats.st_dev     = fs->GetDeviceID();
    m_Stats.st_ino     = fs->GetNextINodeIndex();
    m_Stats.st_nlink   = 1;
    m_Stats.st_mode    = mode;
    m_Stats.st_uid     = process ? process->GetCredentials().euid : 0;
    m_Stats.st_gid     = process ? process->GetCredentials().egid : 0;
    m_Stats.st_rdev    = 0;
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_blocks  = 0;

    // TODO(v1tr10l7): Set to realtime
    m_Stats.st_atim    = {};
    m_Stats.st_mtim    = {};
    m_Stats.st_ctim    = {};

    if (parent && parent->GetStats().st_mode & S_ISGID)
    {
        m_Stats.st_gid = parent->GetStats().st_gid;
        m_Stats.st_mode |= S_ISGID;
    }

    if (S_ISREG(mode))
    {
        m_Capacity = GetDefaultSize();
        m_Data     = new u8[m_Capacity];
    }
}
isize TmpFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);

    if (static_cast<off_t>(offset + bytes) >= m_Stats.st_size)
        bytes = bytes - ((offset + bytes) - m_Stats.st_size);

    Assert(buffer);
    std::memcpy(buffer, reinterpret_cast<u8*>(m_Data) + offset, bytes);
    return bytes;
}
isize TmpFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);

    // TODO(v1tr10l7): we should resize in seperate function
    if (offset + bytes > m_Capacity)
    {
        usize newCapacity = m_Capacity;
        while (offset + bytes >= newCapacity) newCapacity *= 2;

        auto tfs = reinterpret_cast<TmpFs*>(m_Filesystem);
        if (tfs->GetSize() + (newCapacity - m_Capacity) > tfs->GetMaxSize())
        {
            errno = ENOSPC;
            return -1;
        }

        m_Data = static_cast<uint8_t*>(
            KernelHeap::Reallocate(m_Data, newCapacity));
        if (!m_Data) return -1;

        m_Capacity = newCapacity;
    }

    memcpy(m_Data + offset, buffer, bytes);

    if (off_t(offset + bytes) >= m_Stats.st_size)
    {
        m_Stats.st_size = off_t(offset + bytes);
        m_Stats.st_blocks
            = Math::DivRoundUp(m_Stats.st_size, m_Stats.st_blksize);
    }

    return bytes;
}
isize TmpFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);

    if (size > m_Capacity)
    {
        usize newCapacity = m_Capacity;
        while (newCapacity < size) m_Capacity *= 2;

        void* newBuffer = new u8[newCapacity];
        if (!newBuffer) return_err(-1, ENOMEM);

        std::memcpy(newBuffer, m_Data, m_Capacity);
        delete m_Data;
    }

    m_Stats.st_size   = static_cast<off_t>(size);
    m_Stats.st_blocks = Math::DivRoundUp(m_Stats.st_size, m_Stats.st_blksize);

    return 0;
}
