/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/EchFs/EchFsStructures.hpp>
#include <VFS/INode.hpp>

class EchFs;
class EchFsINode : public INode
{
  public:
    EchFsINode(INode* parent, std::string_view name, Filesystem* fs,
               mode_t mode, EchFsDirectoryEntry directoryEntry,
               usize directoryEntryOffset);

    EchFsINode(INode* parent, std::string_view name, Filesystem* fs,
               mode_t mode)
        : EchFsINode(parent, name, fs, mode, {}, 0)
    {
    }

    virtual ~EchFsINode() {}

    virtual std::unordered_map<std::string_view, INode*>&
                  GetChildren() override;

    virtual void  InsertChild(INode* node, std::string_view name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override
    {
        return_err(-1, EROFS);
    }
    virtual ErrorOr<isize> Truncate(usize size) override { return -1; }

    virtual ErrorOr<void>  ChMod(mode_t mode) override { return Error(ENOSYS); }

    friend class EchFs;

  private:
    EchFs*              m_NativeFs = nullptr;
    EchFsDirectoryEntry m_DirectoryEntry;
    usize               m_EntryIndex;
    usize               m_DirectoryEntryOffset;
    std::atomic<usize>  m_NextIndex = 2;

    friend class EchFs;
};
