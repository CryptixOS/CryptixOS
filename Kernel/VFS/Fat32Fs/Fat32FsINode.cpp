/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/Fat32Fs/Fat32Fs.hpp>
#include <VFS/Fat32Fs/Fat32FsINode.hpp>

#include <Time/Time.hpp>

Fat32FsINode::Fat32FsINode(INode* parent, std::string_view name, Filesystem* fs,
                           mode_t mode)
    : INode(parent, name, fs)
{
    m_Stats.st_dev     = fs->GetDeviceID();
    m_Stats.st_ino     = fs->GetNextINodeIndex();
    m_Stats.st_nlink   = 1;
    m_Stats.st_mode    = mode;
    m_Stats.st_uid     = 0;
    m_Stats.st_gid     = 0;
    m_Stats.st_rdev    = fs->GetDeviceID();
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_blocks  = 0;

    m_Stats.st_atim    = Time::GetReal();
    m_Stats.st_ctim    = Time::GetReal();
    m_Stats.st_mtim    = Time::GetReal();
}

std::unordered_map<std::string_view, INode*>& Fat32FsINode::GetChildren()
{
    if (!m_Populated) Populate();
    return m_Children;
}
void Fat32FsINode::InsertChild(INode* node, std::string_view name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}
