/*
 * Created by v1tr10l7 on 28.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Memory/Buffer.hpp>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
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

export module Cryptix:VFS.SynthFsINode;

export import :VFS.INode;

export class SynthFsINode : public INode
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

    ErrorOr<void>                      ResizeBuffer(usize newSize);
};
