/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/DevTmpFs/DevTmpFsINode.hpp>
#include <VFS/Filesystem.hpp>

#include <Time/Time.hpp>

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
        m_Buffer.Resize(PMM::PAGE_SIZE);
        m_Metadata.Size = m_Buffer.Size();
        m_Metadata.BlockCount
            = Math::AlignUp(m_Metadata.Size, m_Metadata.BlockSize);
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

ErrorOr<Ref<DirectoryEntry>>
DevTmpFsINode::CreateNode(Ref<DirectoryEntry> entry, mode_t mode, dev_t dev)
{
    if (m_Children.Contains(entry->Name())) return Error(EEXIST);

    auto maybeINode = m_Filesystem->AllocateNode(entry->Name(), mode);
    RetOnError(maybeINode);

    auto inode    = reinterpret_cast<DevTmpFsINode*>(maybeINode.Value());
    inode->m_Name = entry->Name();
    inode->m_Metadata.Mode = mode;
    if (S_ISREG(mode))
    {
        inode->m_Metadata.Size = PMM::PAGE_SIZE;
        inode->m_Buffer.Resize(PMM::PAGE_SIZE);
    }
    else if (S_ISCHR(mode) || S_ISBLK(mode))
    {
        auto it = DevTmpFs::s_Devices.Find(dev);
        if (it != DevTmpFs::s_Devices.end()) inode->m_Device = it->Value;
    }
    entry->Bind(inode);

    // TODO(v1tr10l7): set dev

    auto currentTime            = Time::GetReal();
    // m_Metadata.Size += TmpFs::DIRECTORY_ENTRY_SIZE;

    m_Metadata.ChangeTime       = currentTime;
    m_Metadata.ModificationTime = currentTime;

    InsertChild(inode, entry->Name());
    if (Mode() & S_ISGID)
    {
        inode->m_Metadata.GID = m_Metadata.GID;
        if (S_ISDIR(mode)) inode->m_Metadata.Mode |= S_ISGID;
    }

    entry->Bind(inode);
    return entry;
}
ErrorOr<Ref<DirectoryEntry>>
DevTmpFsINode::CreateFile(Ref<DirectoryEntry> entry, mode_t mode)
{
    return CreateNode(entry, mode | S_IFREG, 0);
}
ErrorOr<Ref<DirectoryEntry>>
DevTmpFsINode::CreateDirectory(Ref<DirectoryEntry> entry, mode_t mode)
{
    auto maybeEntry = CreateNode(entry, mode | S_IFDIR, 0);
    RetOnError(maybeEntry);

    ++m_Metadata.LinkCount;
    return entry;
}
ErrorOr<Ref<DirectoryEntry>> DevTmpFsINode::Symlink(Ref<DirectoryEntry> entry,
                                                    PathView targetPath)
{
    auto dentry     = TryOrRet(CreateNode(entry, 0777 | S_IFLNK, 0));
    CTOS_UNUSED auto inode      = reinterpret_cast<DevTmpFsINode*>(dentry->INode());

    // inode->m_Target = targetPath;
    return dentry;
}

isize DevTmpFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (m_Device)
    {
        auto result = m_Device->Read(buffer, offset, bytes);
        return result ? result.Value() : -1;
    }

    ScopedLock guard(m_Lock);
    usize      count = bytes;
    if (offset + bytes >= m_Metadata.Size)
        count = bytes - ((offset + bytes) - m_Metadata.Size);

    Memory::Copy(buffer, m_Buffer.Raw() + offset, count);

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
        return result ? result.Value() : -1;
    }

    ScopedLock guard(m_Lock);

    auto       capacity = m_Buffer.Size();
    if (offset + bytes >= capacity)
    {
        usize newCapacity = capacity;
        while (offset + bytes >= newCapacity) newCapacity *= 2;

        m_Buffer.Resize(newCapacity);
    }

    Memory::Copy(m_Buffer.Raw() + offset, buffer, bytes);

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

ErrorOr<isize> DevTmpFsINode::IoCtl(usize request, usize arg)
{
    if (!m_Device) return Error(ENOTTY);

    return m_Device->IoCtl(request, arg);
}
ErrorOr<isize> DevTmpFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);

    auto       capacity = m_Buffer.Size();
    if (m_Device || size == capacity) return 0;

    const Credentials& creds = Process::GetCurrent()->Credentials();
    if (!CanWrite(creds)) return Error(EPERM);

    m_Buffer.Resize(size);
    // m_Buffer.ShrinkToFit();

    if (m_Filesystem->ShouldUpdateCTime())
        m_Metadata.ChangeTime = Time::GetReal();
    if (m_Filesystem->ShouldUpdateMTime())
        m_Metadata.ModificationTime = Time::GetReal();

    m_Metadata.Size = static_cast<off_t>(size);
    m_Metadata.BlockCount
        = Math::DivRoundUp(m_Metadata.Size, m_Metadata.BlockSize);
    return 0;
}
