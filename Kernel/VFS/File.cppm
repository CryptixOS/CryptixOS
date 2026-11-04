/*
 * Created by v1tr10l7 on 30.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
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
#include <Prism/Containers/Deque.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:VFS.File;

export import :Library.UserBuffer;
export import :VFS.INode;

export class INode;
export class DirectoryEntry;
export class File : public RefCounted
{
  public:
    File() = default;
    explicit File(::Ref<class INode> inode);
    virtual ~File() = default;

    virtual class ::Ref<class INode> INode() const;
    virtual usize                    Size() const;
    virtual void                     Close(bool writer) {}

    virtual ErrorOr<File*>           Open(File* file) { return file; }
    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes);
    virtual ErrorOr<isize> Write(const void* src, off_t offset, usize bytes);
    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1);
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1);
    virtual ErrorOr<isize> IoCtl(usize request, usize arg);
    virtual ErrorOr<const stat> Stat() const;
    virtual ErrorOr<isize>      Seek(i32 whence, off_t offset)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> Truncate(off_t size);

    virtual bool           IsCharDevice() const { return false; }
    virtual bool           IsBlockDevice() const { return false; }
    virtual bool           IsFifo() const { return false; }
    virtual bool           IsDirectory() const { return false; }
    virtual bool           IsRegular() const { return false; }
    virtual bool           IsSymlink() const { return false; }
    virtual bool           IsSocket() const { return false; }

  private:
    Spinlock           m_Lock;
    ::Ref<class INode> m_INode = nullptr;
};
