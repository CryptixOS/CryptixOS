/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <VFS/DevTmpFs/DevTmpFsINode.hpp>
#include <VFS/Filesystem.hpp>

#include <Time/Time.hpp>

#include <cstdlib>

DevTmpFsINode::DevTmpFsINode(StringView name, class Filesystem* fs, mode_t mode,
                             Device* device)
    : INode(name, fs)
{
    m_Device                = device;

    m_Metadata.DeviceID     = fs->DeviceID();
    m_Metadata.ID           = fs->NextINodeIndex();
    m_Metadata.LinkCount    = 1;
    m_Metadata.Mode         = mode;
    m_Metadata.UID          = 0;
    m_Metadata.GID          = 0;
    m_Metadata.RootDeviceID = device ? device->ID() : 0;
    m_Metadata.Size         = 0;
    m_Metadata.BlockSize    = 512;
    m_Metadata.BlockCount   = 0;

    if (S_ISREG(mode))
    {
        m_Capacity            = 0x1000;
        m_Data                = new u8[m_Capacity];
        m_Metadata.Size       = m_Capacity;
        m_Metadata.BlockCount = Math::AlignUp(m_Capacity, m_Metadata.BlockSize);
    }

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();
}

ErrorOr<void>
DevTmpFsINode::TraverseDirectories(Ref<class DirectoryEntry> parent,
                                   DirectoryIterator         iterator)
{
    usize offset = 0;
    for (const auto [name, inode] : Children())
    {
        usize  ino  = inode->Stats().st_ino;
        mode_t mode = inode->Stats().st_mode;
        auto   type = IF2DT(mode);

        if (!iterator(name, offset, ino, type)) break;
        ++offset;
    }

    return {};
}
ErrorOr<Ref<DirectoryEntry>> DevTmpFsINode::Lookup(Ref<DirectoryEntry> dentry)
{
    ScopedLock guard(m_Lock);

    auto       child = Children().Find(dentry->Name());
    if (child != Children().end())
    {
        dentry->Bind(child->Value);
        return dentry;
    }

    return Error(ENOENT);
}

isize DevTmpFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (m_Device)
    {
        auto result = m_Device->Read(buffer, offset, bytes);
        return result ? result.value() : -1;
    }

    ScopedLock guard(m_Lock);
    usize      count = bytes;
    if (offset + bytes >= m_Metadata.Size)
        count = bytes - ((offset + bytes) - m_Metadata.Size);

    Memory::Copy(buffer, reinterpret_cast<u8*>(m_Data) + offset, count);

    if (m_Filesystem->ShouldUpdateATime())
        m_Metadata.AccessTime = Time::GetReal();
    return count;
}
isize DevTmpFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (m_Device)
    {
        auto result = m_Device->Write(buffer, offset, bytes);
        return result ? result.value() : -1;
    }

    ScopedLock guard(m_Lock);

    if (offset + bytes >= m_Capacity)
    {
        usize newCapacity = m_Capacity;
        while (offset + bytes >= newCapacity) newCapacity *= 2;

        m_Data = static_cast<u8*>(std::realloc(m_Data, newCapacity));
        if (!m_Data) return_err(-1, ENOMEM);

        m_Capacity = newCapacity;
    }

    Memory::Copy(m_Data + offset, buffer, bytes);

    if (offset + bytes >= m_Metadata.Size)
    {
        m_Metadata.Size = static_cast<off_t>(offset + bytes);
        m_Metadata.BlockCount
            = Math::DivRoundUp(m_Metadata.Size, m_Metadata.BlockSize);
    }

    if (m_Filesystem->ShouldUpdateMTime())
        m_Metadata.ModificationTime = Time::GetReal();
    if (m_Filesystem->ShouldUpdateATime())
        m_Metadata.AccessTime = Time::GetReal();
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

    const Credentials& creds = Process::GetCurrent()->Credentials();
    if (!CanWrite(creds)) return Error(EPERM);

    u8* newData = new u8[size];
    Memory::Copy(newData, m_Data, size > m_Capacity ? size : m_Capacity);

    if (m_Capacity < size)
        Memory::Fill(newData + m_Capacity, 0, size - m_Capacity);
    delete m_Data;

    m_Data     = newData;
    m_Capacity = size;

    if (m_Filesystem->ShouldUpdateCTime())
        m_Metadata.ChangeTime = Time::GetReal();
    if (m_Filesystem->ShouldUpdateMTime())
        m_Metadata.ModificationTime = Time::GetReal();

    m_Metadata.Size = static_cast<off_t>(size);
    m_Metadata.BlockCount
        = Math::DivRoundUp(m_Metadata.Size, m_Metadata.BlockSize);
    return 0;
}
