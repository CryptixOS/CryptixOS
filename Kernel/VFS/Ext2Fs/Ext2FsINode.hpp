/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/Ext2Fs/Ext2FsStructures.hpp>
#include <VFS/INode.hpp>

class Ext2FsINode : public INode
{
  public:
    Ext2FsINode(StringView name, class Ext2Fs* fs, mode_t mode);

    virtual ~Ext2FsINode() {}

    virtual ErrorOr<void>
    TraverseDirectories(::Ref<class DirectoryEntry> parent,
                        DirectoryIterator           iterator) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
                           Lookup(::Ref<DirectoryEntry> dentry) override;

    virtual ErrorOr<File*> Open(class ::Ref<::DirectoryEntry> dentry, i64 flags,
                                u64 accMode) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode,
               dev_t dev = 0) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateFile(::Ref<DirectoryEntry> entry, INodeMode mode) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateDirectory(::Ref<DirectoryEntry> entry, INodeMode mode) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    Symlink(::Ref<DirectoryEntry> entry, PathView targetPath) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    Link(::Ref<DirectoryEntry> oldEntry, ::Ref<DirectoryEntry> entry) override;

    virtual const UnorderedMap<String, ::Ref<INode>>& Children() const
    {
        return m_Children;
    }
    virtual void  InsertChild(::Ref<INode> node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual ErrorOr<Path>  ReadLink() override;
    virtual ErrorOr<isize> Truncate(usize size) override { return -1; }
    virtual ErrorOr<void>  Rename(::Ref<INode> newParent,
                                  StringView   newName) override
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<void> Unlink(::Ref<DirectoryEntry> entry) override;
    virtual ErrorOr<void> RmDir(::Ref<DirectoryEntry> entry) override
    {
        return Error(ENOSYS);
    }

    friend class Ext2Fs;

  private:
    Ext2Fs*                            m_Fs;
    Ext2FsINodeMeta                    m_Meta;
    UnorderedMap<String, ::Ref<INode>> m_Children;
    usize                              m_DirectoryOffset = 0;
    Path                               m_LinkTarget      = ""_p;

    void          Initialize(ino_t index, mode_t mode, u16 type);
    ErrorOr<void> AddDirectoryEntry(Ext2FsDirectoryEntry& dentry);
};
