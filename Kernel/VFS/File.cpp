/*
 * Created by v1tr10l7 on 15.07.2025.
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

module Cryptix:Impl.VFS.File;

import :VFS.File;
import :VFS.INode;

File::File(class ::Ref<class INode> inode)
    : m_INode(inode)
{
}

class ::Ref<class INode> File::INode() const { return m_INode; }
usize          File::Size() const { return m_INode ? m_INode->Size() : 0; }

ErrorOr<isize> File::Read(void* dest, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);
    if (!m_INode)
    {
        LogDebug("File::Read: m_INode is nullptr");
        return Error(ENOENT);
    }
    if (m_INode->IsDirectory()) return Error(EISDIR);

    return m_INode->Read(dest, offset, bytes);
}
ErrorOr<isize> File::Write(const void* src, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);
    if (m_INode->IsDirectory()) return Error(EISDIR);

    return m_INode->Write(src, offset, bytes);
}
ErrorOr<isize> File::Read(const UserBuffer& out, usize count, isize offset)
{
    ScopedLock guard(m_Lock);
    if (m_INode->IsDirectory()) return Error(EISDIR);

    return m_INode->Read(out.Raw(), offset, count);
}

ErrorOr<isize> File::Write(const UserBuffer& in, usize count, isize offset)
{
    ScopedLock guard(m_Lock);
    if (m_INode->IsDirectory()) return Error(EISDIR);

    return m_INode->Write(in.Raw(), offset, count);
}
ErrorOr<isize> File::IoCtl(usize request, usize arg)
{
    if (!m_INode) return Error(ENOSYS);
    return m_INode->IoCtl(request, arg);
}

ErrorOr<const stat> File::Stat() const
{
    if (!m_INode) return Error(ENOENT);

    return m_INode->Stats();
}

ErrorOr<isize> File::Truncate(off_t size) { return m_INode->Truncate(size); }
