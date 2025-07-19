/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/Allocator/KernelHeap.hpp>
#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <Time/Time.hpp>

#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>

TmpFsINode::TmpFsINode(class Filesystem* fs)
    : INode(fs)
{
    m_Metadata.DeviceID         = fs->DeviceID();
    m_Metadata.ID               = fs->NextINodeIndex();
    m_Metadata.LinkCount        = 1;
    m_Metadata.RootDeviceID     = 0;
    m_Metadata.Size             = 0;
    m_Metadata.BlockSize        = 512;
    m_Metadata.BlockCount       = 0;

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
}
TmpFsINode::TmpFsINode(StringView name, class Filesystem* fs, mode_t mode,
                       uid_t uid, gid_t gid)
    : INode(name, fs)
{
    m_Metadata.DeviceID         = fs->DeviceID();
    m_Metadata.ID               = fs->NextINodeIndex();
    m_Metadata.LinkCount        = 1;
    m_Metadata.Mode             = mode;
    m_Metadata.UID              = uid;
    m_Metadata.GID              = gid;
    m_Metadata.RootDeviceID     = 0;
    m_Metadata.Size             = 0;
    m_Metadata.BlockSize        = 512;
    m_Metadata.BlockCount       = 0;

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();

    // if (parent && parent->Stats().Mode & S_ISGID)
    // {
    //     m_Metadata.GID = parent->Stats().st_gid;
    //     m_Metadata.Mode |= S_ISGID;
    // }

    if (S_ISREG(mode))
    {
        m_Capacity      = GetDefaultSize();
        m_Metadata.Size = GetDefaultSize();
        m_Data          = new u8[m_Capacity];
    }
}

ErrorOr<void> TmpFsINode::TraverseDirectories(Ref<class DirectoryEntry> parent,
                                              DirectoryIterator iterator)
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

ErrorOr<Ref<DirectoryEntry>> TmpFsINode::Lookup(Ref<DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);

    auto       child = Children().Find(entry->Name());
    if (child != Children().end())
    {
        entry->Bind(child->Value);
        return entry;
    }

    return Error(ENOENT);
}
INode* TmpFsINode::Lookup(const String& name)
{
    ScopedLock guard(m_Lock);

    auto       child = Children().Find(name);
    if (child != Children().end()) return child->Value;

    return nullptr;
}
void TmpFsINode::InsertChild(INode* inode, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = inode;
}

isize TmpFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);

    if (offset + bytes >= m_Metadata.Size)
        bytes = bytes - ((offset + bytes) - m_Metadata.Size);

    Assert(buffer);
    if (m_Filesystem->ShouldUpdateATime())
        m_Metadata.AccessTime = Time::GetReal();
    Memory::Copy(buffer, reinterpret_cast<u8*>(m_Data) + offset, bytes);
    return bytes;
}
isize TmpFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);

    // TODO(v1tr10l7): we should resize in separate function
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

    if (offset + bytes >= m_Metadata.Size)
    {
        m_Metadata.Size = off_t(offset + bytes);
        m_Metadata.BlockCount
            = Math::DivRoundUp(m_Metadata.Size, m_Metadata.BlockSize);
    }

    if (m_Filesystem->ShouldUpdateATime())
        m_Metadata.AccessTime = Time::GetReal();
    if (m_Filesystem->ShouldUpdateMTime())
        m_Metadata.ModificationTime = Time::GetReal();
    m_Metadata.Size = m_Capacity;
    return bytes;
}
ErrorOr<isize> TmpFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);
    if (size == m_Capacity) return 0;

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

ErrorOr<void> TmpFsINode::Rename(INode* newParent, StringView newName)
{
    // TODO(v1tr10l7): Remove old inode

    // auto parent = reinterpret_cast<TmpFsINode*>(m_Parent);
    // parent->m_Children.Erase(m_Name);

    m_Name = newName;
    newParent->InsertChild(this, Name());

    return {};
}

ErrorOr<Ref<DirectoryEntry>> TmpFsINode::CreateNode(Ref<DirectoryEntry> entry,
                                                    mode_t mode, dev_t dev)
{
    if (m_Children.Contains(entry->Name())) return Error(EEXIST);

    auto maybeINode = m_Filesystem->AllocateNode(entry->Name(), mode);
    RetOnError(maybeINode);

    auto inode             = reinterpret_cast<TmpFsINode*>(maybeINode.value());
    inode->m_Name          = entry->Name();
    inode->m_Metadata.Mode = mode;
    if (S_ISREG(mode))
    {
        inode->m_Capacity      = inode->GetDefaultSize();
        inode->m_Metadata.Size = inode->GetDefaultSize();
        inode->m_Data          = new u8[inode->m_Capacity];
    }

    // TODO(v1tr10l7): set dev

    auto currentTime = Time::GetReal();
    m_Metadata.Size += TmpFs::DIRECTORY_ENTRY_SIZE;

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

ErrorOr<Ref<DirectoryEntry>> TmpFsINode::CreateFile(Ref<DirectoryEntry> entry,
                                                    mode_t              mode)
{
    return CreateNode(entry, mode | S_IFREG, 0);
}
ErrorOr<Ref<DirectoryEntry>>
TmpFsINode::CreateDirectory(Ref<DirectoryEntry> entry, mode_t mode)
{

    auto maybeEntry = CreateNode(entry, mode | S_IFDIR, 0);
    RetOnError(maybeEntry);

    ++m_Metadata.LinkCount;
    return entry;
}
ErrorOr<Ref<DirectoryEntry>> TmpFsINode::Link(Ref<DirectoryEntry> oldEntry,
                                              Ref<DirectoryEntry> entry)
{
    auto inode = reinterpret_cast<TmpFsINode*>(oldEntry->INode());
    if (inode->IsDirectory()) return Error(EPERM);

    m_Metadata.Size += TmpFs::DIRECTORY_ENTRY_SIZE;

    auto currentTime             = Time::GetReal();
    m_Metadata.ChangeTime        = currentTime;
    m_Metadata.ModificationTime  = currentTime;
    inode->m_Metadata.ChangeTime = currentTime;
    ++inode->m_Metadata.LinkCount;

    entry->Bind(inode);
    return entry;
}

ErrorOr<void> TmpFsINode::Unlink(Ref<DirectoryEntry> entry)
{
    auto it = m_Children.Find(entry->Name());
    if (it == m_Children.end()) return {};

    auto inode = reinterpret_cast<TmpFsINode*>(entry->INode());
    m_Metadata.Size -= TmpFs::DIRECTORY_ENTRY_SIZE;

    auto currentTime             = Time::GetReal();
    m_Metadata.ChangeTime        = currentTime;
    m_Metadata.ModificationTime  = currentTime;
    inode->m_Metadata.ChangeTime = currentTime;
    --inode->m_Metadata.LinkCount;

    ScopedLock guard(m_Lock);
    if (inode->m_Metadata.LinkCount == 0)
    {
        m_Children.Erase(it);
        delete entry->INode();
    }

    auto parent = entry->Parent();
    parent->RemoveChild(entry);

    LogTrace("VFS::TmpFs: Unlinked inode => `{}`, link count => {}",
             entry->Name(), inode->m_Metadata.LinkCount);
    return {};
}
ErrorOr<void> TmpFsINode::RmDir(Ref<DirectoryEntry> entry)
{
    --m_Metadata.LinkCount;

    return Unlink(entry);
}

ErrorOr<void> TmpFsINode::Link(PathView path)
{
    // auto pathRes = VFS::ResolvePath(VFS::GetRootNode(), path);
    // if (pathRes.Node) return Error(EEXIST);

    // auto parent = pathRes.Parent;
    // parent->InsertChild(this, pathRes.BaseName);

    // return {};
    return Error(ENOSYS);
}
