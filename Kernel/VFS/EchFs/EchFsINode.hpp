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
    EchFsINode(StringView name, class Filesystem* fs, mode_t mode,
               EchFsDirectoryEntry directoryEntry, usize directoryEntryOffset);

    EchFsINode(StringView name, class Filesystem* fs, mode_t mode)
        : EchFsINode(name, fs, mode, {}, 0)
    {
    }

    virtual ~EchFsINode() {}

    virtual const std::unordered_map<StringView, INode*>& Children() const;

    virtual void  InsertChild(INode* node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override
    {
        return_err(-1, EROFS);
    }
    virtual ErrorOr<isize> Truncate(usize size) override { return -1; }
    virtual ErrorOr<void>  ChMod(mode_t mode) override { return Error(ENOSYS); }

    friend class EchFs;

  private:
    EchFs*                                 m_NativeFs = nullptr;
    EchFsDirectoryEntry                    m_DirectoryEntry;
    usize                                  m_EntryIndex;
    usize                                  m_DirectoryEntryOffset;
    Atomic<usize>                          m_NextIndex = 2;

    std::unordered_map<StringView, INode*> m_Children;
    friend class EchFs;
};
