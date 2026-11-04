/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Memory/Memory.hpp>
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
#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Core/TypeTraits.hpp>
#include <API/Posix/sys/statfs.h>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/Array.hpp>

module Cryptix:Impl.VFS.Fat32Fs.Fat32FsINode;

import :VFS.Fat32Fs.Fat32Fs;
import :VFS.Fat32Fs.Fat32FsINode;
import :Time.Time;

Fat32FsINode::Fat32FsINode(StringView name, class Filesystem* fs, mode_t mode)
    : INode(name, fs)
    , m_Fat32Fs(reinterpret_cast<Fat32Fs*>(fs))
{
    m_Metadata.DeviceID         = fs->BackingDeviceID();
    m_Metadata.ID               = fs->NextINodeIndex();
    m_Metadata.LinkCount        = 1;
    m_Metadata.Mode             = mode;
    m_Metadata.UID              = 0;
    m_Metadata.GID              = 0;
    m_Metadata.RootDeviceID     = 0;
    // m_Metadata.Size    = S_ISDIR(mode) ? m_Filesystem->GetClusterSize() : 0;
    // m_Metadata.BlockSize = m_Filesystem->GetClusterSize();
    m_Metadata.BlockCount       = S_ISDIR(mode);

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();

    m_DirectoryOffset           = -1;
    m_Cluster                   = 0;

    Fat32DirectoryEntry entry;
    if (S_ISDIR(mode))
    {
        u32 cluster = m_Fat32Fs->AllocateClusters(1);
        if (!cluster)
        {
            errno = ENOSPC;
            return;
        }

        m_Cluster        = cluster;
        entry.Attributes = Fat32Attribute::eDirectory;
        // entry.SetCluster(cluster);
    }

    // m_DirectoryOffset
    //     = m_Filesystem->InsertDirectoryEntry(m_Parent, entry, name);
    if (m_DirectoryOffset == uintptr_t(-1)) { return; }

    if (S_ISDIR(mode))
    {
        m_Populated = true;
        Memory::Copy(entry.Name, ".          ", 11);
        //
    }
}

const UnorderedMap<StringView, ::Ref<INode>>& Fat32FsINode::Children() const
{
    // TODO(v1tr10l7): Populate records
    return m_Children;
}
void Fat32FsINode::InsertChild(::Ref<INode> node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}

isize Fat32FsINode::Read(void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);

    usize      endOffset = offset + bytes;
    if (endOffset > m_Metadata.Size)
    {
        endOffset = m_Metadata.Size;
        bytes
            = static_cast<usize>(offset) >= endOffset ? 0 : endOffset - offset;
    }

    if (bytes == 0) return 0;

    if (!m_Fat32Fs->ReadBytes(m_Cluster, reinterpret_cast<u8*>(buffer), offset,
                              bytes))
        return_err(-1, ENOENT);

    return bytes;
}
