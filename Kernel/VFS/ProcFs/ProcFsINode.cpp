/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/ProcFs/ProcFsINode.hpp>

ProcFsINode::ProcFsINode(INode* parent, std::string_view name, Filesystem* fs,
                         mode_t mode, ProcFsProperty* property)
    : INode(parent, name, fs)
    , m_Property(property)
{
    Assert(!S_ISDIR(mode) || !m_Property);

    m_Stats.st_dev     = m_Filesystem->GetDeviceID();
    m_Stats.st_ino     = m_Filesystem->GetNextINodeIndex();
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
        m_Property->String = m_Property->GenerateProperty();
        m_Stats.st_size    = m_Property->String.length();
    }
    return m_Stats;
}

void ProcFsINode::InsertChild(INode* node, std::string_view name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}
isize ProcFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    std::string& property    = *m_Property;

    usize        bytesCopied = property.copy(reinterpret_cast<char*>(buffer),
                                             std::min(bytes, property.length()));
    property                 = m_Property->GenerateProperty();
    return bytesCopied;
}
isize ProcFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    return -1;
}
isize ProcFsINode::Truncate(usize size) { return -1; }
