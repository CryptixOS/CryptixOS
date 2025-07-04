/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/KernelHeap.hpp>
#include <Scheduler/Process.hpp>

#include <Prism/Utility/Math.hpp>
#include <Time/Time.hpp>

#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>

TmpFsINode::TmpFsINode(class Filesystem* fs)
    : INode(fs)
{
    m_Stats.st_dev     = fs->DeviceID();
    m_Stats.st_ino     = fs->NextINodeIndex();
    m_Stats.st_nlink   = 1;
    m_Stats.st_rdev    = 0;
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_blocks  = 0;

    m_Stats.st_atim    = Time::GetReal();
    m_Stats.st_mtim    = Time::GetReal();
    m_Stats.st_ctim    = Time::GetReal();
}
TmpFsINode::TmpFsINode(StringView name, class Filesystem* fs, mode_t mode,
                       uid_t uid, gid_t gid)
    : INode(name, fs)
{
    m_Stats.st_dev     = fs->DeviceID();
    m_Stats.st_ino     = fs->NextINodeIndex();
    m_Stats.st_nlink   = 1;
    m_Stats.st_mode    = mode;
    m_Stats.st_uid     = uid;
    m_Stats.st_gid     = gid;
    m_Stats.st_rdev    = 0;
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_blocks  = 0;

    m_Stats.st_atim    = Time::GetReal();
    m_Stats.st_mtim    = Time::GetReal();
    m_Stats.st_ctim    = Time::GetReal();

    // if (parent && parent->Stats().st_mode & S_ISGID)
    // {
    //     m_Stats.st_gid = parent->Stats().st_gid;
    //     m_Stats.st_mode |= S_ISGID;
    // }

    if (S_ISREG(mode))
    {
        m_Capacity      = GetDefaultSize();
        m_Stats.st_size = GetDefaultSize();
        m_Data          = new u8[m_Capacity];
    }
}

ErrorOr<void> TmpFsINode::TraverseDirectories(class DirectoryEntry* parent,
                                              DirectoryIterator     iterator)
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
INode* TmpFsINode::Lookup(const String& name)
{
    ScopedLock guard(m_Lock);

    auto       child = Children().find(name);
    if (child != Children().end()) return child->second;

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

    if (static_cast<off_t>(offset + bytes) >= m_Stats.st_size)
        bytes = bytes - ((offset + bytes) - m_Stats.st_size);

    Assert(buffer);
    if (m_Filesystem->ShouldUpdateATime()) m_Stats.st_atim = Time::GetReal();
    std::memcpy(buffer, reinterpret_cast<u8*>(m_Data) + offset, bytes);
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

    if (off_t(offset + bytes) >= m_Stats.st_size)
    {
        m_Stats.st_size = off_t(offset + bytes);
        m_Stats.st_blocks
            = Math::DivRoundUp(m_Stats.st_size, m_Stats.st_blksize);
    }

    if (m_Filesystem->ShouldUpdateATime()) m_Stats.st_atim = Time::GetReal();
    if (m_Filesystem->ShouldUpdateMTime()) m_Stats.st_mtim = Time::GetReal();
    m_Stats.st_size = m_Capacity;
    return bytes;
}
ErrorOr<isize> TmpFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);
    if (size == m_Capacity) return 0;

    const Credentials& creds = Process::GetCurrent()->Credentials();
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

ErrorOr<void> TmpFsINode::Rename(INode* newParent, StringView newName)
{
    // TODO(v1tr10l7): Remove old inode

    // auto parent = reinterpret_cast<TmpFsINode*>(m_Parent);
    // parent->m_Children.erase(m_Name);

    m_Name = newName;
    newParent->InsertChild(this, Name());

    return {};
}

ErrorOr<Ref<DirectoryEntry>> TmpFsINode::CreateNode(Ref<DirectoryEntry> entry,
                                                    mode_t mode, dev_t dev)
{
    if (m_Children.contains(entry->Name())) return Error(EEXIST);

    auto maybeINode = m_Filesystem->AllocateNode();
    RetOnError(maybeINode);

    auto inode    = reinterpret_cast<TmpFsINode*>(maybeINode.value());
    inode->m_Name = entry->Name();
    inode->m_Attributes.Mode = mode;
    inode->m_Stats.st_mode   = mode;
    if (S_ISREG(mode))
    {
        inode->m_Capacity      = inode->GetDefaultSize();
        inode->m_Stats.st_size = inode->GetDefaultSize();
        inode->m_Data          = new u8[inode->m_Capacity];
    }

    // TODO(v1tr10l7): set dev

    auto currentTime = Time::GetReal();
    m_Attributes.Size += TmpFs::DIRECTORY_ENTRY_SIZE;

    m_Attributes.ChangeTime       = currentTime;
    m_Attributes.ModificationTime = currentTime;
    entry->Bind(inode);

    InsertChild(inode, entry->Name());
    if (Mode() & S_ISGID)
    {
        inode->m_Attributes.GID = m_Attributes.GID;
        if (S_ISDIR(mode)) inode->m_Attributes.Mode |= S_ISGID;
    }
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

    ++m_Stats.st_nlink;
    return entry;
}
ErrorOr<Ref<DirectoryEntry>> TmpFsINode::Link(Ref<DirectoryEntry> oldEntry,
                                              Ref<DirectoryEntry> entry)
{
    auto inode = reinterpret_cast<TmpFsINode*>(oldEntry->INode());
    if (inode->IsDirectory()) return Error(EPERM);

    m_Stats.st_size += TmpFs::DIRECTORY_ENTRY_SIZE;

    auto currentTime               = Time::GetReal();
    m_Attributes.ChangeTime        = currentTime;
    m_Attributes.ModificationTime  = currentTime;
    inode->m_Attributes.ChangeTime = currentTime;
    ++inode->m_Stats.st_nlink;

    entry->Bind(inode);
    return entry;
}

ErrorOr<void> TmpFsINode::Unlink(Ref<DirectoryEntry> entry)
{
    auto it = m_Children.find(entry->Name());
    if (it == m_Children.end()) return {};

    auto inode = reinterpret_cast<TmpFsINode*>(entry->INode());
    m_Stats.st_size -= TmpFs::DIRECTORY_ENTRY_SIZE;

    auto currentTime               = Time::GetReal();
    m_Attributes.ChangeTime        = currentTime;
    m_Attributes.ModificationTime  = currentTime;
    inode->m_Attributes.ChangeTime = currentTime;
    --inode->m_Stats.st_nlink;

    ScopedLock guard(m_Lock);
    if (inode->m_Stats.st_nlink == 0)
    {
        m_Children.erase(it);
        delete entry->INode();
    }

    auto parent = entry->Parent();
    parent->RemoveChild(entry);

    LogTrace("VFS::TmpFs: Unlinked inode => `{}`, link count => {}",
             entry->Name(), inode->m_Stats.st_nlink);
    return {};
}
ErrorOr<void> TmpFsINode::RmDir(Ref<DirectoryEntry> entry)
{
    --m_Stats.st_nlink;

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
