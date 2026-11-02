/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
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
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Memory/Buffer.hpp>

export module Cryptix:VFS.DevPtsFs.DevPtsFsINode;

export import :Drivers.Core.Device;
export import :VFS.INode;
export import :VFS.SynthFsINode;

export using DeviceID = DeviceID;

export class DevPtsFsINode final : public INode, public NonCopyable<DevPtsFsINode>
{
  public:
    DevPtsFsINode(StringView name, class Filesystem* fs, INodeID id,
                  INodeMode mode, Device* device = nullptr);
    virtual ~DevPtsFsINode() {}

    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode,
               ::DeviceID dev) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateFile(::Ref<DirectoryEntry> entry, mode_t mode) override
    {
        return Error(ENOENT);
    }
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateDirectory(::Ref<DirectoryEntry> entry, mode_t mode) override
    {
        return Error(ENOENT);
    }

    virtual ErrorOr<::Ref<DirectoryEntry>> Symlink(::Ref<DirectoryEntry> entry,
                                                   PathView targetPath) override
    {
        return Error(ENOENT);
    }

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Link(::Ref<DirectoryEntry> oldEntry, ::Ref<DirectoryEntry> entry) override
    {
        return Error(ENOENT);
    }

    virtual void  InsertChild(::Ref<INode> node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> IoCtl(usize request, usize arg) override;
    virtual ErrorOr<Path>  ReadLink() override { return Error(ENOENT); }

    virtual ErrorOr<isize> Truncate(usize size) override
    {
        return Error(ENOENT);
    }
    virtual ErrorOr<void> Rename(::Ref<INode> newParent,
                                 StringView   newName) override
    {
        return Error(ENOENT);
    }

    virtual ErrorOr<void> Unlink(::Ref<DirectoryEntry> entry) override
    {
        return Error(ENOENT);
    }

    virtual ErrorOr<void> RmDir(::Ref<DirectoryEntry> entry) override
    {
        return Error(ENOENT);
    }

  private:
    Device*                                m_Device = nullptr;
    UnorderedMap<StringView, ::Ref<INode>> m_Children;

    friend class DevPtsFs;
};
