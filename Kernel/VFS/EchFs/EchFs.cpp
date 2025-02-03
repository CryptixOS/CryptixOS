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

constexpr const char* ECHFS_SIGNATURE   = "_ECH_FS_";

constexpr usize       ROOT_DIRECTORY_ID = 0xffff'ffff'ffff'ffff;

INode*                EchFs::Mount(INode* parent, INode* source, INode* target,
                                   std::string_view name, void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    EchFsIdentityTable* identityTable = new EchFsIdentityTable;
    if (!identityTable) return nullptr;

    usize allocationTableOffset = 0, allocationTableSize = 0;
    if (!source)
    {
        delete identityTable;
        return nullptr;
    }
    m_Device = source;
    source->Read(identityTable, 0, sizeof(EchFsIdentityTable));

    if (std::strncmp(reinterpret_cast<char*>(identityTable->Signature),
                     ECHFS_SIGNATURE, 8))
    {
        delete identityTable;
        return nullptr;
    }

    m_BlockSize           = identityTable->BytesPerBlock;
    allocationTableOffset = 16;
    allocationTableSize
        = (identityTable->TotalBlockCount * sizeof(u64) + m_BlockSize - 1)
        / m_BlockSize;

    m_MainDirectoryOffset = allocationTableOffset + allocationTableSize;
    m_MainDirectoryLength = identityTable->MainDirectoryLength;

    EchFsDirectoryEntry dirEntry;
    usize               offset = m_MainDirectoryOffset * m_BlockSize;
    source->Read(&dirEntry, offset, sizeof(EchFsDirectoryEntry));

    EchFsDirectoryEntry rootDirectory;
    if (dirEntry.DirectoryID != ROOT_DIRECTORY_ID) rootDirectory = dirEntry;
    while (dirEntry.DirectoryID)
    {
        source->Read(&dirEntry, offset, sizeof(EchFsDirectoryEntry));
        if (dirEntry.DirectoryID) LogInfo("Entry: {}", (char*)dirEntry.Name);
        offset += sizeof(EchFsDirectoryEntry);
    }

    m_Root = new EchFsINode(parent, name, this, 0644 | S_IFDIR, rootDirectory,
                            m_MainDirectoryOffset * m_BlockSize);
    if (m_Root) m_MountedOn = target;

    delete identityTable;
    return m_Root;
}

INode* EchFs::CreateNode(INode* parent, std::string_view name, mode_t mode)
{
    return nullptr; // new EchFsINode(parent, name, this, mode);
}

void EchFs::InsertDirectoryEntries(class EchFsINode* node)
{
    usize               offset = node->m_DirectoryEntryOffset;
    EchFsDirectoryEntry entry{};
    m_Device->Read(&entry, offset, sizeof(EchFsDirectoryEntry));

    while (entry.DirectoryID)
    {
        mode_t type = 0;
        if (entry.ObjectType == 0) type = S_IFREG;
        else if (entry.ObjectType == 1) type = S_IFDIR;
        node->m_Children[std::string((char*)entry.Name)] = new EchFsINode(
            node, (char*)entry.Name, this, 0644 | type, entry, offset);
        offset += sizeof(EchFsDirectoryEntry);
        m_Device->Read(&entry, offset, sizeof(EchFsDirectoryEntry));
    }
}
