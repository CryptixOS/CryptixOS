/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Prism/Core/Types.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:VFS.Ext2Fs.Ext2FsINode;

export import :VFS.Ext2Fs.Ext2FsStructures;
export import :VFS.INode;

export class Ext2FsINode : public INode
{
  public:
    Ext2FsINode(StringView name, class Ext2Fs* fs, mode_t mode);
    Ext2FsINode(StringView name, class Ext2Fs* fs, INodeID ino, INodeMode mode);

    virtual ~Ext2FsINode() {}

    virtual ErrorOr<void>
    TraverseDirectories(::Ref<class DirectoryEntry> parent,
                        DirectoryIterator           iterator) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    Lookup(::Ref<DirectoryEntry> dentry) override;

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
