/*
 * Created by v1tr10l7 on 01.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/EchFs/EchFs.hpp>
#include <VFS/EchFs/EchFsINode.hpp>

EchFsINode::EchFsINode(INode* parent, StringView name, Filesystem* fs,
                       mode_t mode, EchFsDirectoryEntry directoryEntry,
                       usize directoryEntryOffset)
    : INode(parent, name, fs)
    , m_DirectoryEntry(directoryEntry)
    , m_DirectoryEntryOffset(directoryEntryOffset)
{
    m_Stats.st_size    = m_Stats.st_blocks * sizeof(EchFsDirectoryEntry);
    m_Stats.st_blksize = sizeof(EchFsDirectoryEntry);
    m_EntryIndex       = m_NextIndex++;

    m_NativeFs         = reinterpret_cast<EchFs*>(fs);
}

std::unordered_map<StringView, INode*>& EchFsINode::GetChildren()
{
    reinterpret_cast<EchFs*>(m_Filesystem)->Populate(this);
    return m_Children;
}

void EchFsINode::InsertChild(INode* node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}

isize EchFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    return m_NativeFs->ReadDirectoryEntry(
        m_DirectoryEntry, reinterpret_cast<u8*>(buffer), offset, bytes);
}
