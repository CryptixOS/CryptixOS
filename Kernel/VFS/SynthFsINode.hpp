/*
 * Created by v1tr10l7 on 28.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Vector.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <System/Limits.hpp>

#include <VFS/INode.hpp>

class SynthFsINode : public INode
{
  public:
    SynthFsINode(StringView name, class Filesystem* fs, INodeID id,
                 INodeMode mode);
    virtual ~SynthFsINode() = default;

    virtual ErrorOr<void>
    TraverseDirectories(::Ref<class DirectoryEntry> parent,
                        DirectoryIterator           iterator) override;

    virtual ErrorOr<::Ref<DirectoryEntry>>
                                  Lookup(::Ref<DirectoryEntry> dentry) override;

    inline static constexpr usize GetDefaultSize() { return 0x1000; }

    virtual const UnorderedMap<String, ::Ref<INode>>& Children() const
    {
        return m_Children;
    }

    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateNode(::Ref<DirectoryEntry> entry, mode_t mode, dev_t dev) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateFile(::Ref<DirectoryEntry> entry, mode_t mode) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateDirectory(::Ref<DirectoryEntry> entry, mode_t mode) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    Symlink(::Ref<DirectoryEntry> entry, PathView targetPath) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    Link(::Ref<DirectoryEntry> oldEntry, ::Ref<DirectoryEntry> entry) override;

    virtual void  InsertChild(::Ref<INode> node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<Path>  ReadLink() override;

    virtual ErrorOr<isize> Truncate(usize size) override;
    virtual ErrorOr<void>  Rename(::Ref<INode> newParent,
                                  StringView   newName) override;

    virtual ErrorOr<void>  Unlink(::Ref<DirectoryEntry> entry) override;
    virtual ErrorOr<void>  RmDir(::Ref<DirectoryEntry> entry) override;

    virtual ErrorOr<isize> SetExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                                StringView            name,
                                                const u8* value, usize size,
                                                isize flags) override;
    virtual ErrorOr<isize> GetExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                                StringView name, u8* value,
                                                usize size) override;
    virtual ErrorOr<isize> ListExtendedAttributes(::Ref<DirectoryEntry> dentry,
                                                  char*                 list,
                                                  usize size) override;
    virtual ErrorOr<isize> RemoveExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                                   StringView name) override;

    virtual ErrorOr<void>  FlushMetadata() override
    {
        m_Dirty = false;
        return {};
    }

  protected:
    inline static constexpr usize      DIRECTORY_ENTRY_SIZE = 20;

    Buffer                             m_Buffer;
    UnorderedMap<String, ::Ref<INode>> m_Children;
    String                             m_Target = ""_s;

    // Values are stored at their real size (up to XATTR_MAX_VALUE_SIZE)
    UnorderedMap<String, Vector<u8>> m_ExtendedAttributes;

    ErrorOr<void> ResizeBuffer(usize newSize);
};
