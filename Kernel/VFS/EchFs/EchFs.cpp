/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/PMM.hpp>
#include <Prism/Utility/Math.hpp>

#include <Time/Time.hpp>
#include <VFS/INode.hpp>

#include <VFS/EchFs/EchFs.hpp>
#include <VFS/EchFs/EchFsINode.hpp>

constexpr const char* ECHFS_SIGNATURE = "_ECH_FS_";

EchFs::~EchFs()
{
    if (m_Root) VFS::RecursiveDelete(m_Root);
    if (m_IdentityTable)
    {
        delete m_IdentityTable;
        m_IdentityTable = nullptr;
    }
}

INode* EchFs::Mount(INode* parent, INode* source, INode* target,
                    StringView name, const void* data)
{
    if (!source) return nullptr;

    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    m_IdentityTable = new EchFsIdentityTable;
    if (!m_IdentityTable) return nullptr;
    m_SourceDevice = source;
    m_DeviceID     = source->GetStats().st_rdev;

    StringView signature;
    usize      totalBlockCount = 0;
    usize      mainDirectoryLength;
    u64        uuid[2];

    if (source->Read(m_IdentityTable, 0, sizeof(EchFsIdentityTable))
        != sizeof(EchFsIdentityTable))
    {
        LogError("EchFs: Failed to read the identity table");
        goto fail_free_id_table;
    }
    signature = {reinterpret_cast<const char*>(m_IdentityTable->Signature), 8};

    totalBlockCount     = m_IdentityTable->TotalBlockCount;
    mainDirectoryLength = m_IdentityTable->MainDirectoryLength;
    m_BlockSize         = m_IdentityTable->BytesPerBlock;
    uuid[0]             = m_IdentityTable->UUID[0];
    uuid[1]             = m_IdentityTable->UUID[1];

    EchFsDebug("EchFs", "", "IdentityTable =>");
    EchFsDebug("Signature", "", signature);
    EchFsDebug("TotalBlockCount", ":#x", totalBlockCount);
    EchFsDebug("MainDirectoryLength", ":#x", mainDirectoryLength);
    EchFsDebug("BytesPerBlock", ":#x", m_BlockSize);
    EchFsDebug("UUID0", ":#x", uuid[0]);
    EchFsDebug("UUID1", ":#x", uuid[1]);

    if (signature != ECHFS_SIGNATURE)
    {
        LogError("EchFs: Invalid identity table signature => '{}'", signature);
        goto fail_free_id_table;
    }

    m_BytesLimit            = m_IdentityTable->TotalBlockCount * m_BlockSize;

    m_AllocationTableOffset = 16;
    m_AllocationTableSize
        = (totalBlockCount * sizeof(u64) + m_BlockSize - 1) / m_BlockSize;

    m_MainDirectoryStart
        = (m_AllocationTableOffset + m_AllocationTableSize) * m_BlockSize;
    m_MainDirectoryEnd
        = m_MainDirectoryStart + mainDirectoryLength * m_BlockSize;

    m_NativeRoot = new EchFsINode(parent, "/", this, 0644 | S_IFDIR);
    m_Root       = m_NativeRoot;

    if (!m_NativeRoot) goto fail_free_id_table;

    if (source->Read(&m_NativeRoot->m_DirectoryEntry, m_MainDirectoryStart,
                     sizeof(EchFsDirectoryEntry))
        != sizeof(EchFsDirectoryEntry))
    {
        LogError("EchFs: Failed to read root directory entry");
        goto fail_free_inode_and_id_table;
    }

    m_NativeRoot->m_DirectoryEntryOffset = m_MainDirectoryStart;
    if (m_NativeRoot->m_DirectoryEntry.ParentID != ECHFS_ROOT_DIRECTORY_ID)
    {
        LogError("EchFs: Corrupted root directory entry");
        goto fail_free_inode_and_id_table;
    }

    m_NativeRoot->m_Stats.st_dev     = GetDeviceID();
    m_NativeRoot->m_Stats.st_ino     = 2;
    m_NativeRoot->m_Stats.st_mode    = 0644 | S_IFDIR;
    m_NativeRoot->m_Stats.st_nlink   = 2;
    m_NativeRoot->m_Stats.st_uid     = 0;
    m_NativeRoot->m_Stats.st_gid     = 0;
    m_NativeRoot->m_Stats.st_rdev    = 0;
    m_NativeRoot->m_Stats.st_size    = totalBlockCount * m_BlockSize;
    m_NativeRoot->m_Stats.st_blksize = m_BlockSize;
    m_NativeRoot->m_Stats.st_blocks  = totalBlockCount;

    m_NativeRoot->m_Stats.st_ctim    = Time::GetReal();
    m_NativeRoot->m_Stats.st_atim    = Time::GetReal();
    m_NativeRoot->m_Stats.st_mtim    = Time::GetReal();

    m_MountedOn                      = target;
    return m_NativeRoot;
fail_free_inode_and_id_table:
    delete m_NativeRoot;
fail_free_id_table:
    delete m_IdentityTable;
    m_IdentityTable = nullptr;
    return nullptr;
}

INode* EchFs::CreateNode(INode* parent, StringView name, mode_t mode)
{
    return nullptr;
}

bool EchFs::Populate(INode* node)
{
    EchFsINode*         inode      = reinterpret_cast<EchFsINode*>(node);
    EchFsDirectoryEntry inodeEntry = inode->m_DirectoryEntry;
    ScopedLock          guard(m_Lock);

    // TODO(v1tr10l7): Traversing whole main directory, in order to read all
    // children is incredibely slow, so we should introduce some caching,
    // to make it a bit more efficient
    EchFsDirectoryEntry entry{};
    for (usize offset = inode->m_DirectoryEntryOffset;
         offset < m_MainDirectoryEnd; offset += sizeof(EchFsDirectoryEntry))
    {
        m_SourceDevice->Read(&entry, offset, sizeof(EchFsDirectoryEntry));
        StringView name = reinterpret_cast<const char*>(entry.Name);
        if (name.Empty()) continue;

        u64    inodeDirectoryID = inodeEntry.Payload;
        mode_t mode             = 0644;

        if (entry.ObjectType == EchFsDirectoryEntryType::eRegular)
            mode |= S_IFREG;
        else if (entry.ObjectType == EchFsDirectoryEntryType::eDirectory)
        {
            if (inode->IsFilesystemRoot())
                inodeDirectoryID = ECHFS_ROOT_DIRECTORY_ID;
            mode |= S_IFDIR;
        }
        if (entry.ParentID != inodeDirectoryID) continue;

        EchFsINode* child
            = new EchFsINode(inode, name, this, mode, entry, offset);
        child->m_Stats.st_dev     = GetDeviceID();
        child->m_Stats.st_ino     = 2;
        child->m_Stats.st_mode    = mode;
        child->m_Stats.st_nlink   = 1;
        child->m_Stats.st_uid     = entry.OwnerID;
        child->m_Stats.st_gid     = entry.GroupID;
        child->m_Stats.st_rdev    = 0;
        child->m_Stats.st_size    = entry.FileSize;
        child->m_Stats.st_blksize = m_BlockSize;
        child->m_Stats.st_blocks
            = Math::DivRoundUp(entry.FileSize, m_BlockSize);

        child->m_Stats.st_ctim = Time::GetReal();
        child->m_Stats.st_atim = Time::GetReal();
        child->m_Stats.st_mtim = Time::GetReal();

        inode->InsertChild(child, name);
    }

    return (inode->m_Populated = true);
}

isize EchFs::ReadDirectoryEntry(EchFsDirectoryEntry& entry, u8* dest,
                                isize offset, usize bytes)
{
    Assert(entry.ObjectType == EchFsDirectoryEntryType::eRegular);

    if (static_cast<usize>(offset) > entry.FileSize) return 0;
    if (static_cast<usize>(offset) + bytes > entry.FileSize)
        bytes = entry.FileSize - offset;

    isize nread = m_SourceDevice->Read(
        dest, entry.Payload * m_BlockSize + offset, bytes);

    return nread;
}
