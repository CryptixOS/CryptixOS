/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/KernelHeap.hpp>

#include <Utility/Math.hpp>

#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>

TmpFsINode::TmpFsINode(INode* parent, std::string_view name, Filesystem* fs,
                       mode_t mode)
    : INode(parent, name, fs)
{
    m_Stats.st_size    = 0;
    m_Stats.st_blocks  = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_dev     = fs->GetDeviceID();
    m_Stats.st_ino     = fs->GetNextINodeIndex();
    m_Stats.st_mode    = mode;
    m_Stats.st_nlink   = 1;

    // TODO(v1tr10l7): Set to realtime
    m_Stats.st_atim    = {};
    m_Stats.st_ctim    = {};
    m_Stats.st_mtim    = {};

    if (S_ISREG(mode))
    {
        capacity = GetDefaultSize();
        data     = new u8[capacity];
    }
}
isize TmpFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);

    if (!buffer) return_err(-1, EFAULT);
    usize count = bytes;
    if (static_cast<off_t>(offset + bytes) >= m_Stats.st_size)
        count = bytes - ((offset + bytes) - m_Stats.st_size);

    Assert(buffer);
    std::memcpy(buffer, reinterpret_cast<uint8_t*>(data) + offset, count);
    return count;
}
isize TmpFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);

    if (offset + bytes > capacity)
    {
        usize newCapacity = capacity;
        while (offset + bytes >= newCapacity) newCapacity *= 2;

        auto tfs = reinterpret_cast<TmpFs*>(m_Filesystem);
        if (tfs->currentSize + (newCapacity - capacity) > tfs->maxSize)
        {
            errno = ENOSPC;
            return -1;
        }

        data = static_cast<uint8_t*>(KernelHeap::Reallocate(data, newCapacity));
        if (!data) return -1;

        capacity = newCapacity;
    }

    memcpy(data + offset, buffer, bytes);

    if (off_t(offset + bytes) >= m_Stats.st_size)
    {
        m_Stats.st_size = off_t(offset + bytes);
        m_Stats.st_blocks
            = Math::DivRoundUp(m_Stats.st_size, m_Stats.st_blksize);
    }

    return bytes;
}
