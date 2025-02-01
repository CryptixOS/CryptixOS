/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/PMM.hpp>

#include <VFS/INode.hpp>

#include <VFS/EchFs/EchFs.hpp>
#include <VFS/EchFs/EchFsINode.hpp>

constexpr const char* ECHFS_SIGNATURE = "_ECH_FS_";

struct EchFsDirectoryEntry
{
    u64 DirectoryID;
    u8  ObjectType;
    u8  Name[201];
    u64 UnixAtime;
    u64 UnixMtime;
    u64 Permissions;
    u16 OwnerID;
    u16 GroupID;
    u64 UnixCtime;
    u64 StartingBlock;
    u64 FileSize;
} __attribute__((packed));

constexpr usize ROOT_DIRECTORY_ID = 0xffff'ffff'ffff'ffff;

INode*          EchFs::Mount(INode* parent, INode* source, INode* target,
                             std::string_view name, void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    EchFsIdentityTable* identityTable = new EchFsIdentityTable;
    if (!identityTable) return nullptr;

    usize allocationTableOffset = 0, allocationTableSize = 0;
    if (!source) goto cleanup;
    source->Read(identityTable, 0, sizeof(EchFsIdentityTable));

    if (std::strncmp(reinterpret_cast<char*>(identityTable->Signature),
                     ECHFS_SIGNATURE, 8))
        goto cleanup;

    m_BlockSize           = identityTable->BytesPerBlock;
    allocationTableOffset = 16;
    allocationTableSize
        = (identityTable->TotalBlockCount * sizeof(u64) + m_BlockSize - 1)
        / m_BlockSize;

    m_MainDirectoryOffset = allocationTableOffset + allocationTableSize;
    m_MainDirectoryLength = identityTable->MainDirectoryLength;
    m_Root                = CreateNode(parent, name, 0644 | S_IFDIR);
    if (m_Root) m_MountedOn = target;

cleanup:
    delete identityTable;
    return m_Root;
}

INode* EchFs::CreateNode(INode* parent, std::string_view name, mode_t mode)
{
    return new EchFsINode(parent, name, this, mode,
                          m_MainDirectoryOffset * m_BlockSize);
}
