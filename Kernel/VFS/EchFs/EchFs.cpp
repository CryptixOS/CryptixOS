/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Debug/Debug.hpp>

#include <Memory/PMM.hpp>
#include <Prism/Utility/Math.hpp>

#include <Time/Time.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <VFS/EchFs/EchFs.hpp>
#include <VFS/EchFs/EchFsINode.hpp>

constexpr StringView ECHFS_SIGNATURE = "_ECH_FS_"_sv;

EchFs::~EchFs()
{
    // TODO(v1tr10l7): if (m_Root) VFS::RecursiveDelete(m_Root);
    if (m_IdentityTable)
    {
        delete m_IdentityTable;
        m_IdentityTable = nullptr;
    }
}

ErrorOr<::Ref<DirectoryEntry>> EchFs::Mount(StringView  sourcePath,
                                          const void* data)
{
    auto pathResolution
        = VFS::ResolvePath(nullptr, sourcePath).ValueOr(VFS::PathResolution{});

    auto sourceEntry = pathResolution.Entry;
    if (!sourceEntry || !sourceEntry->INode()) return nullptr;
    auto source = sourceEntry->INode();

    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    m_IdentityTable = new EchFsIdentityTable;
    if (!m_IdentityTable) return nullptr;
    m_SourceDevice = source;
    m_DeviceID     = source->Stats().st_rdev;

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

#if CTOS_DEBUG_ECHFS
    EchFsDebug("EchFs", "", "IdentityTable =>");
    EchFsDebug("Signature", "", signature);
    EchFsDebug("TotalBlockCount", ":#x", totalBlockCount);
    EchFsDebug("MainDirectoryLength", ":#x", mainDirectoryLength);
    EchFsDebug("BytesPerBlock", ":#x", m_BlockSize);
    EchFsDebug("UUID0", ":#x", uuid[0]);
    EchFsDebug("UUID1", ":#x", uuid[1]);
#endif

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

    m_RootEntry  = new DirectoryEntry(nullptr, "/");
    m_NativeRoot = new EchFsINode("/", reinterpret_cast<Filesystem*>(this),
                                  0644 | S_IFDIR);
    m_Root       = reinterpret_cast<INode*>(m_NativeRoot);

    if (!m_NativeRoot) goto fail_free_id_table;
    m_RootEntry->Bind(m_Root);

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

    m_NativeRoot->m_Metadata.DeviceID
        = reinterpret_cast<Filesystem*>(this)->DeviceID();
    m_NativeRoot->m_Metadata.ID               = 2;
    m_NativeRoot->m_Metadata.Mode             = 0644 | S_IFDIR;
    m_NativeRoot->m_Metadata.LinkCount        = 2;
    m_NativeRoot->m_Metadata.UID              = 0;
    m_NativeRoot->m_Metadata.GID              = 0;
    m_NativeRoot->m_Metadata.RootDeviceID     = 0;
    m_NativeRoot->m_Metadata.Size             = totalBlockCount * m_BlockSize;
    m_NativeRoot->m_Metadata.BlockSize        = m_BlockSize;
    m_NativeRoot->m_Metadata.BlockCount       = totalBlockCount;

    m_NativeRoot->m_Metadata.ChangeTime       = Time::GetReal();
    m_NativeRoot->m_Metadata.AccessTime       = Time::GetReal();
    m_NativeRoot->m_Metadata.ModificationTime = Time::GetReal();

    return m_RootEntry;
fail_free_inode_and_id_table:
    delete m_NativeRoot;
fail_free_id_table:
    delete m_IdentityTable;
    m_IdentityTable = nullptr;
    return nullptr;
}

ErrorOr<INode*> EchFs::CreateNode(INode* parent, ::Ref<DirectoryEntry> entry,
                                  mode_t mode, uid_t uid, gid_t gid)
{
    return nullptr;
}

bool EchFs::Populate(EchFsINode* native)
{
    INode*              inode      = reinterpret_cast<INode*>(native);
    EchFsDirectoryEntry inodeEntry = native->m_DirectoryEntry;
    ScopedLock          guard(m_Lock);

    // TODO(v1tr10l7): Traversing whole main directory, in order to read all
    // children is incredibely slow, so we should introduce some caching,
    // to make it a bit more efficient
    EchFsDirectoryEntry entry{};
    for (usize offset = native->m_DirectoryEntryOffset;
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

        EchFsINode* child = new EchFsINode(
            name, reinterpret_cast<Filesystem*>(this), mode, entry, offset);
        child->m_Metadata.DeviceID
            = reinterpret_cast<Filesystem*>(this)->DeviceID();
        child->m_Metadata.ID           = 2;
        child->m_Metadata.Mode         = mode;
        child->m_Metadata.LinkCount    = 1;
        child->m_Metadata.UID          = entry.OwnerID;
        child->m_Metadata.GID          = entry.GroupID;
        child->m_Metadata.RootDeviceID = 0;
        child->m_Metadata.Size         = entry.FileSize;
        child->m_Metadata.BlockSize    = m_BlockSize;
        child->m_Metadata.BlockCount
            = Math::DivRoundUp(entry.FileSize, m_BlockSize);

        child->m_Metadata.ChangeTime       = Time::GetReal();
        child->m_Metadata.AccessTime       = Time::GetReal();
        child->m_Metadata.ModificationTime = Time::GetReal();

        inode->InsertChild(reinterpret_cast<INode*>(child), name);
    }

    return (native->m_Populated = true);
}
bool EchFs::Populate(DirectoryEntry* entry)
{
    if (!entry) return_err(false, ENOENT);
    auto inode = entry->INode();

    return Populate(reinterpret_cast<EchFsINode*>(inode));
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
