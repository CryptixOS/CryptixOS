/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/INode.hpp>

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

class EchFsINode : public INode
{
  public:
    EchFsINode(INode* parent, std::string_view name, Filesystem* fs,
               mode_t mode, EchFsDirectoryEntry directoryEntry,
               usize directoryEntryOffset)
        : INode(parent, name, fs)
        , m_DirectoryEntry(directoryEntry)
        , m_DirectoryEntryOffset(directoryEntryOffset)
    {
        (void)m_DirectoryEntry;
    }

    virtual ~EchFsINode() {}

    virtual std::unordered_map<std::string_view, INode*>&
                  GetChildren() override;

    virtual void  InsertChild(INode* node, std::string_view name) override {}
    virtual isize Read(void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual ErrorOr<isize> Truncate(usize size) override { return -1; }

    friend class EchFs;

  private:
    EchFsDirectoryEntry m_DirectoryEntry;
    usize               m_DirectoryEntryOffset;
};
