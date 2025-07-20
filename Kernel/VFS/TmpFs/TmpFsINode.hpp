/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Logger.hpp>
#include <Prism/Memory/Buffer.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/INode.hpp>

extern bool g_LogTmpFs;

#define CTOS_DEBUG_TMPFS 1
#if CTOS_DEBUG_TMPFS != 0
    #define TmpFsTrace(...) if (g_LogTmpFs) LogMessage("[\e[35mTmpFs\e[0m]: {}\n", __VA_ARGS__)
#else
    #define TmpFsTrace(...) 
#endif

class TmpFsINode final : public INode
{
  public:
    TmpFsINode(class Filesystem* fs);
    TmpFsINode(StringView name, class Filesystem* fs, mode_t mode,
               uid_t uid = 0, gid_t gid = 0);

    virtual ~TmpFsINode()
    {
    }

    virtual ErrorOr<void>
    TraverseDirectories(Ref<class DirectoryEntry> parent,
                        DirectoryIterator         iterator) override;

    virtual ErrorOr<Ref<DirectoryEntry>>
                                  Lookup(Ref<DirectoryEntry> dentry) override;
    virtual INode*                Lookup(const String& name) override;

    inline static constexpr usize GetDefaultSize() { return 0x1000; }

    virtual const UnorderedMap<StringView, INode*>& Children() const
    {
        return m_Children;
    }
    virtual void  InsertChild(INode* node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

    virtual ErrorOr<void> Rename(INode* newParent, StringView newName) override;

    virtual ErrorOr<Ref<DirectoryEntry>>
    CreateNode(Ref<DirectoryEntry> entry, mode_t mode, dev_t dev) override;
    virtual ErrorOr<Ref<DirectoryEntry>> CreateFile(Ref<DirectoryEntry> entry,
                                                    mode_t mode) override;
    virtual ErrorOr<Ref<DirectoryEntry>>
    CreateDirectory(Ref<DirectoryEntry> entry, mode_t mode) override;
    virtual ErrorOr<Ref<DirectoryEntry>>
    Link(Ref<DirectoryEntry> oldEntry, Ref<DirectoryEntry> entry) override;

    virtual ErrorOr<void> Unlink(Ref<DirectoryEntry> entry) override;
    virtual ErrorOr<void> RmDir(Ref<DirectoryEntry> entry) override;
    virtual ErrorOr<void> Link(PathView path) override;

  private:
    Buffer                           m_Buffer;
    UnorderedMap<StringView, INode*> m_Children;

    friend class TmpFs;
};
