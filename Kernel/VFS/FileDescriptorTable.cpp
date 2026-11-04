/*
 * Created by v1tr10l7 on 22.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
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
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.FileDescriptorTable;

import :VFS.FileDescriptorTable;
import :VFS.VFS;

FileDescriptorTable::FileDescriptorTable() { m_NextIndex = 1; }
isize FileDescriptorTable::Insert(::Ref<FileDescriptor> fd, isize desired)
{
    ScopedLock guard(m_Lock);
    isize      fdNum = m_NextIndex;

    auto       found = m_Table.Find(desired);
    if (desired >= 0 && found == m_Table.end()) fdNum = desired;
    m_Table[fdNum] = fd;

    ++m_NextIndex;
    return fdNum;
}
isize FileDescriptorTable::Replace(::Ref<FileDescriptor> fd, isize desired)
{
    ScopedLock guard(m_Lock);
    m_Table[desired] = fd;

    return desired;
}
isize FileDescriptorTable::Erase(isize fdNum)
{
    ScopedLock            guard(m_Lock);
    ::Ref<FileDescriptor> fd = GetFd(fdNum);
    if (!fd) return_err(-1, EBADF);

    if (fd->DirectoryEntry()
        && fd->DirectoryEntry()->Name().Contains("ctl.sock"))
        LogTrace("Erasing auroractl.sock => {}", fd->DirectoryEntry()->Path());
    m_Table.Erase(fdNum);
    return 0;
}

void FileDescriptorTable::Clear()
{
    m_Table.Clear();
    m_NextIndex = 0;
}
