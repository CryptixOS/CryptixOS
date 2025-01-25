/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Time/Time.hpp>
#include <Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <VFS/DevTmpFs/DevTmpFsINode.hpp>

#include <cstdlib>

DevTmpFsINode::DevTmpFsINode(INode* parent, std::string_view name,
                             Filesystem* fs, mode_t mode, Device* device)
    : INode(parent, name, fs)
{
    m_Device           = device;

    m_Stats.st_dev     = fs->GetDeviceID();
    m_Stats.st_ino     = fs->GetNextINodeIndex();
    m_Stats.st_nlink   = 1;
    m_Stats.st_mode    = mode;
    m_Stats.st_uid     = 0;
    m_Stats.st_gid     = 0;
    m_Stats.st_rdev    = device ? device->GetID() : 0;
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_blocks  = 0;

    if (S_ISREG(mode))
    {
        m_Capacity        = 0x1000;
        m_Data            = new u8[m_Capacity];
        m_Stats.st_size   = m_Capacity;
        m_Stats.st_blocks = Math::AlignUp(m_Capacity, m_Stats.st_blksize);
    }

    m_Stats.st_atim = Time::GetReal();
    m_Stats.st_ctim = Time::GetReal();
    m_Stats.st_mtim = Time::GetReal();
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

    if (m_Filesystem->ShouldUpdateATime()) m_Stats.st_atim = Time::GetReal();
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

    if (m_Filesystem->ShouldUpdateMTime()) m_Stats.st_mtim = Time::GetReal();
    if (m_Filesystem->ShouldUpdateATime()) m_Stats.st_atim = Time::GetReal();
    return bytes;
}

i32 DevTmpFsINode::IoCtl(usize request, usize arg)
{
    if (!m_Device) return_err(-1, ENOTTY);

    return m_Device->IoCtl(request, arg);
}
ErrorOr<isize> DevTmpFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);
    if (m_Device || size == m_Capacity) return 0;

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
