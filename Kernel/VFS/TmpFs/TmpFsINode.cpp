/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/KernelHeap.hpp>
#include <Scheduler/Process.hpp>

#include <Time/Time.hpp>
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

    m_Stats.st_atim    = Time::GetReal();
    m_Stats.st_mtim    = Time::GetReal();
    m_Stats.st_ctim    = Time::GetReal();

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
ErrorOr<isize> TmpFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);
    if (size == m_Capacity) return 0;

    const Credentials& creds = Process::GetCurrent()->GetCredentials();
    if (!CanWrite(creds)) return Error(EPERM);

    u8* newData = new u8[size];
    std::memcpy(newData, m_Data, size > m_Capacity ? size : m_Capacity);

    if (m_Capacity < size)
        std::memset(newData + m_Capacity, 0, size - m_Capacity);
    delete m_Data;

    m_Data     = newData;
    m_Capacity = size;

    if (m_Filesystem->ShouldUpdateCTime()) m_Stats.st_ctim = Time::GetReal();
    if (m_Filesystem->ShouldUpdateMTime()) m_Stats.st_mtim = Time::GetReal();

    m_Stats.st_size   = static_cast<off_t>(size);
    m_Stats.st_blocks = Math::DivRoundUp(m_Stats.st_size, m_Stats.st_blksize);

    return 0;
}
