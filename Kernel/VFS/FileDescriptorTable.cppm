/*
 * Created by v1tr10l7 on 27.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Containers/UnorderedMap.hpp>
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
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:VFS.FileDescriptorTable;

export import :VFS.FileDescriptor;

export class FileDescriptorTable : public RefCounted
{
    using TableType = UnorderedMap<isize, ::Ref<FileDescriptor>>;

  public:
    FileDescriptorTable();

    isize       Insert(::Ref<FileDescriptor> descriptor, isize desired = -1);
    isize       Replace(::Ref<FileDescriptor> descriptor, isize desired);
    isize       Erase(isize fdNum);

    void        Clear();

    inline bool IsValid(isize fd) const { return m_Table.Contains(fd); }
    inline ::Ref<FileDescriptor> GetFd(isize fd) const
    {
        if (!IsValid(fd)) return nullptr;

        return m_Table.At(fd);
    }

    auto                          begin() { return m_Table.begin(); }
    auto                          end() { return m_Table.end(); }

    inline ::Ref<FileDescriptor>& operator[](usize i) { return m_Table[i]; }

  private:
    Spinlock      m_Lock;
    TableType     m_Table;

    Atomic<isize> m_NextIndex = 0;
};
