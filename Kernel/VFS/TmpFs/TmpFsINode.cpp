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
ErrorOr<void> TmpFsINode::MkDir(StringView name, mode_t mode, uid_t uid,
                                gid_t gid)
{
    if (m_Children.contains(name)) return Error(EEXIST);
    auto umask = Process::Current()->Umask();
    mode &= ~umask & 0777;

    auto entry = new class DirectoryEntry(name);

    auto inodeOr
        = m_Filesystem->CreateNode(this, entry, mode | S_IFDIR, uid, gid);
    auto inode = reinterpret_cast<TmpFsINode*>(inodeOr.value());
    if (!inode) return Error(errno);

    InsertChild(inode, inode->Name());
    return {};
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
ErrorOr<void> TmpFsINode::ChMod(mode_t mode)
{
    m_Stats.st_mode &= ~0777;
    m_Stats.st_mode |= mode & 0777;

    return {};
}
