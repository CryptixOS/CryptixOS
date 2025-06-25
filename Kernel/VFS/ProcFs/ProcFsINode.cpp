/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/dirent.h>

#include <Memory/PMM.hpp>
#include <VFS/ProcFs/ProcFsINode.hpp>

isize ProcFsProperty::Read(u8* outBuffer, off_t offset, usize size)
{
    if (static_cast<usize>(offset) >= Buffer.Size()) return 0;

    usize bytesCopied = Buffer.Copy(reinterpret_cast<char*>(outBuffer) + offset,
                                    std::min(size, Buffer.Size() - offset));

    if (offset + bytesCopied >= Buffer.Size()) GenerateRecord();
    return bytesCopied;
}

ProcFsINode::ProcFsINode(INode* parent, StringView name, Filesystem* fs,
                         mode_t mode, ProcFsProperty* property)
    : INode(parent, name, fs)
    , m_Property(property)
{
    Assert(!S_ISDIR(mode) || !m_Property);

    m_Stats.st_dev     = m_Filesystem->DeviceID();
    m_Stats.st_ino     = m_Filesystem->NextINodeIndex();
    m_Stats.st_nlink   = 1;
    m_Stats.st_mode    = mode;
    m_Stats.st_uid     = 0;
    m_Stats.st_gid     = 0;
    m_Stats.st_rdev    = m_Stats.st_dev;
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_blocks  = 0;
}

const stat& ProcFsINode::GetStats()
{
    if (m_Property)
    {
        m_Property->GenerateRecord();
        m_Stats.st_size = m_Property->Buffer.Size();
    }
    return m_Stats;
}
ErrorOr<void> ProcFsINode::TraverseDirectories(DirectoryIterator iterator)
{
    usize offset = 0;
    for (const auto [name, inode] : Children())
    {
        usize  ino  = inode->GetStats().st_ino;
        mode_t mode = inode->GetStats().st_mode;
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
