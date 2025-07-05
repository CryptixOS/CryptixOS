/*
 * Created by v1tr10l7 on 01.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/EchFs/EchFs.hpp>
#include <VFS/EchFs/EchFsINode.hpp>

EchFsINode::EchFsINode(StringView name, class Filesystem* fs, mode_t mode,
                       EchFsDirectoryEntry directoryEntry,
                       usize               directoryEntryOffset)
    : INode(name, fs)
    , m_DirectoryEntry(directoryEntry)
    , m_DirectoryEntryOffset(directoryEntryOffset)
{
    m_Metadata.Size      = m_Metadata.BlockCount * sizeof(EchFsDirectoryEntry);
    m_Metadata.BlockSize = sizeof(EchFsDirectoryEntry);
    m_EntryIndex         = m_NextIndex++;

    m_NativeFs           = reinterpret_cast<EchFs*>(fs);
}

const std::unordered_map<StringView, INode*>& EchFsINode::Children() const
{
    auto inode = const_cast<EchFsINode*>(this);
    reinterpret_cast<EchFs*>(m_Filesystem)->Populate(inode);
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
