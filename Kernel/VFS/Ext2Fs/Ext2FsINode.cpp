/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Ext2Fs/Ext2FsINode.hpp>

#include <Time/Time.hpp>

Ext2FsINode::Ext2FsINode(INode* parent, std::string_view name, Ext2Fs* fs,
                         mode_t mode)
    : INode(parent, name, fs)
    , m_Fs(fs)
{
    m_Stats.st_dev     = fs->GetDeviceID();
    m_Stats.st_ino     = fs->GetNextINodeIndex();
    m_Stats.st_mode    = mode;
    m_Stats.st_nlink   = 1;
    m_Stats.st_uid     = 0;
    m_Stats.st_gid     = 0;
    m_Stats.st_rdev    = 0;
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = m_Fs->GetBlockSize();
    m_Stats.st_blocks  = 0;

    m_Stats.st_atim    = Time::GetReal();
    m_Stats.st_ctim    = Time::GetReal();
    m_Stats.st_mtim    = Time::GetReal();
}

void Ext2FsINode::InsertChild(INode* node, std::string_view name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}
isize Ext2FsINode::Read(void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);
    m_Fs->ReadINodeEntry(&m_Meta, m_Stats.st_ino);

    if (static_cast<isize>(offset + bytes) > m_Stats.st_size)
        bytes = bytes - ((offset + bytes) - m_Stats.st_size);

    m_Stats.st_atim   = Time::GetReal();
    m_Meta.AccessTime = m_Stats.st_atim.tv_sec;
    m_Fs->WriteINodeEntry(m_Meta, m_Stats.st_ino);

    return m_Fs->ReadINode(m_Meta, reinterpret_cast<u8*>(buffer), offset,
                           bytes);
}

ErrorOr<void> Ext2FsINode::ChMod(mode_t mode)
{
    ScopedLock guard(m_Lock);
    m_Fs->ReadINodeEntry(&m_Meta, m_Stats.st_ino);

    m_Meta.Permissions &= ~0777;
    m_Meta.Permissions |= mode & 0777;

    m_Fs->WriteINodeEntry(m_Meta, m_Stats.st_ino);

    m_Stats.st_mode &= ~0777;
    m_Stats.st_mode |= mode & 0777;

    return {};
}
