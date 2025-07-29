/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Memory/Memory.hpp>

#include <VFS/Fat32Fs/Fat32Fs.hpp>
#include <VFS/Fat32Fs/Fat32FsINode.hpp>

#include <Time/Time.hpp>

Fat32FsINode::Fat32FsINode(StringView name, class Filesystem* fs, mode_t mode)
    : INode(name, fs)
    , m_Fat32Fs(reinterpret_cast<Fat32Fs*>(fs))
{
    m_Metadata.DeviceID         = fs->BackingDeviceID();
    m_Metadata.ID               = fs->NextINodeIndex();
    m_Metadata.LinkCount        = 1;
    m_Metadata.Mode             = mode;
    m_Metadata.UID              = 0;
    m_Metadata.GID              = 0;
    m_Metadata.RootDeviceID     = 0;
    // m_Metadata.Size    = S_ISDIR(mode) ? m_Filesystem->GetClusterSize() : 0;
    // m_Metadata.BlockSize = m_Filesystem->GetClusterSize();
    m_Metadata.BlockCount       = S_ISDIR(mode);

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();

    m_DirectoryOffset           = -1;
    m_Cluster                   = 0;

    Fat32DirectoryEntry entry;
    if (S_ISDIR(mode))
    {
        u32 cluster = m_Fat32Fs->AllocateClusters(1);
        if (!cluster)
        {
            errno = ENOSPC;
            return;
        }

        m_Cluster        = cluster;
        entry.Attributes = Fat32Attribute::eDirectory;
        // entry.SetCluster(cluster);
    }

    // m_DirectoryOffset
    //     = m_Filesystem->InsertDirectoryEntry(m_Parent, entry, name);
    if (m_DirectoryOffset == uintptr_t(-1)) { return; }

    if (S_ISDIR(mode))
    {
        m_Populated = true;
        Memory::Copy(entry.Name, ".          ", 11);
        //
    }
}

const UnorderedMap<StringView, INode*>& Fat32FsINode::Children() const
{
    // TODO(v1tr10l7): Populate records
    return m_Children;
}
void Fat32FsINode::InsertChild(INode* node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}

isize Fat32FsINode::Read(void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);

    usize      endOffset = offset + bytes;
    if (endOffset > m_Metadata.Size)
    {
        endOffset = m_Metadata.Size;
        bytes
            = static_cast<usize>(offset) >= endOffset ? 0 : endOffset - offset;
    }

    if (bytes == 0) return 0;

    if (!m_Fat32Fs->ReadBytes(m_Cluster, reinterpret_cast<u8*>(buffer), offset,
                              bytes))
        return_err(-1, ENOENT);

    return bytes;
}
