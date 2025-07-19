/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/dirent.h>
#include <Memory/PMM.hpp>

#include <VFS/Filesystem.hpp>
#include <VFS/ProcFs/ProcFsINode.hpp>

isize ProcFsProperty::Read(u8* outBuffer, off_t offset, usize size)
{
    if (static_cast<usize>(offset) >= Buffer.Size()) return 0;

    usize bytesCopied = Buffer.Copy(reinterpret_cast<char*>(outBuffer) + offset,
                                    std::min(size, Buffer.Size() - offset));

    if (offset + bytesCopied >= Buffer.Size()) GenerateRecord();
    return bytesCopied;
}

ProcFsINode::ProcFsINode(StringView name, class Filesystem* fs, mode_t mode,
                         ProcFsProperty* property)
    : INode(name, fs)
    , m_Property(property)
{
    Assert(!S_ISDIR(mode) || !m_Property);

    m_Metadata.DeviceID     = m_Filesystem->DeviceID();
    m_Metadata.ID           = m_Filesystem->NextINodeIndex();
    m_Metadata.LinkCount    = 1;
    m_Metadata.Mode         = mode;
    m_Metadata.UID          = 0;
    m_Metadata.GID          = 0;
    m_Metadata.RootDeviceID = m_Metadata.DeviceID;
    m_Metadata.Size         = 0;
    m_Metadata.BlockSize    = 512;
    m_Metadata.BlockCount   = 0;
}

const stat ProcFsINode::Stats()
{
    if (m_Property)
    {
        m_Property->GenerateRecord();
        m_Metadata.Size = m_Property->Buffer.Size();
    }

    stat stats{};
    stats.st_dev     = m_Metadata.DeviceID;
    stats.st_ino     = m_Metadata.ID;
    stats.st_nlink   = m_Metadata.LinkCount;
    stats.st_mode    = m_Metadata.Mode;
    stats.st_uid     = m_Metadata.UID;
    stats.st_gid     = m_Metadata.GID;
    stats.st_rdev    = m_Metadata.RootDeviceID;
    stats.st_size    = m_Metadata.Size;
    stats.st_blksize = m_Metadata.BlockSize;
    stats.st_blocks  = m_Metadata.BlockCount;
    stats.st_atim    = m_Metadata.AccessTime;
    stats.st_mtim    = m_Metadata.ModificationTime;
    stats.st_ctim    = m_Metadata.ChangeTime;
    return stats;
}
ErrorOr<void> ProcFsINode::TraverseDirectories(Ref<class DirectoryEntry> parent,
                                               DirectoryIterator iterator)
{
    usize offset = 0;
    for (const auto [name, inode] : Children())
    {
        usize  ino  = inode->Stats().st_ino;
        mode_t mode = inode->Stats().st_mode;
        auto   type = IF2DT(mode);

        if (iterator(name, offset, ino, type)) break;
        ++offset;
    }

    return {};
}

void ProcFsINode::InsertChild(INode* node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}
isize ProcFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    u8* dest = reinterpret_cast<u8*>(buffer);

    return m_Property ? m_Property->Read(dest, offset, bytes) : -1;
}
isize ProcFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    return -1;
}
ErrorOr<isize> ProcFsINode::Truncate(usize size) { return Error(EROFS); }
