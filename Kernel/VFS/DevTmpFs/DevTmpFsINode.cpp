/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Utility/Math.hpp>
#include <VFS/DevTmpFs/DevTmpFsINode.hpp>

#include <cstdlib>

DevTmpFsINode::DevTmpFsINode(INode* parent, std::string_view name,
                             Filesystem* fs, mode_t mode, Device* device)
    : INode(parent, name, fs)
{
    m_Device           = device;

    m_Stats.st_size    = 0;
    m_Stats.st_blocks  = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_dev     = fs->GetDeviceID();
    m_Stats.st_ino     = fs->GetNextINodeIndex();
    m_Stats.st_mode    = mode;
    m_Stats.st_nlink   = 1;

    if (S_ISREG(mode))
    {
        m_Capacity = 0x1000;
        m_Data     = new u8[m_Capacity];
    }

    // TODO(v1tr10l7): Set to realtime
    m_Stats.st_atim = {};
    m_Stats.st_ctim = {};
    m_Stats.st_mtim = {};
}

isize DevTmpFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (m_Device) return m_Device->Read(buffer, offset, bytes);

    ScopedLock guard(m_Lock);
    usize      count = bytes;
    if (static_cast<off_t>(offset + bytes) >= m_Stats.st_size)
        count = bytes - ((offset + bytes) - m_Stats.st_size);

    std::memcpy(buffer, reinterpret_cast<u8*>(m_Data) + offset, count);
    return count;
}
isize DevTmpFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (m_Device) return m_Device->Write(buffer, offset, bytes);

    ScopedLock guard(m_Lock);

    if (offset + bytes >= m_Capacity)
    {
        usize newCapacity = m_Capacity;
        while (offset + bytes >= newCapacity) newCapacity *= 2;

        m_Data = static_cast<u8*>(std::realloc(m_Data, newCapacity));
        if (!m_Data) return_err(-1, ENOMEM);

        m_Capacity = newCapacity;
    }

    std::memcpy(m_Data + offset, buffer, bytes);

    if (static_cast<off_t>(offset + bytes) >= m_Stats.st_size)
    {
        m_Stats.st_size = static_cast<off_t>(offset + bytes);
        m_Stats.st_blocks
            = Math::DivRoundUp(m_Stats.st_size, m_Stats.st_blksize);
    }

    return bytes;
}

i32 DevTmpFsINode::IoCtl(usize request, usize arg)
{
    if (!m_Device) return_err(-1, ENOTTY);

    return m_Device->IoCtl(request, arg);
}
