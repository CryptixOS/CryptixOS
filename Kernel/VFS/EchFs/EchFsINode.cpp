/*
 * Created by v1tr10l7 on 01.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <VFS/EchFs/EchFsStructures.hpp>
#include <API/Posix/termios.h>
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
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.EchFs.EchFsINode;

import :VFS.EchFs.EchFs;
import :VFS.EchFs.EchFsINode;

EchFsINode::EchFsINode(StringView name, class Filesystem* fs, mode_t mode,
                       EchFsDirectoryEntry directoryEntry,
                       usize               directoryEntryOffset)
    : INode(name, fs)
    , m_DirectoryEntry(directoryEntry)
    , m_DirectoryEntryOffset(directoryEntryOffset)
{
    m_Metadata.Size      = m_Metadata.BlockCount * sizeof(EchFsDirectoryEntry);
    m_Metadata.BlockSize = sizeof(EchFsDirectoryEntry);
    m_EntryIndex         = m_NextIndex++;

    m_NativeFs           = reinterpret_cast<EchFs*>(fs);
}

const UnorderedMap<StringView, ::Ref<INode>>& EchFsINode::Children() const
{
    auto inode = const_cast<EchFsINode*>(this);
    reinterpret_cast<EchFs*>(m_Filesystem)->Populate(inode);
    return m_Children;
}

void EchFsINode::InsertChild(::Ref<INode> node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}

isize EchFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    return m_NativeFs->ReadDirectoryEntry(
        m_DirectoryEntry, reinterpret_cast<u8*>(buffer), offset, bytes);
}
