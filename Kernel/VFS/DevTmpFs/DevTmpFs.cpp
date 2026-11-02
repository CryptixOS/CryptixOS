/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Memory.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Utility/Time.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
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
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
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
#include <API/Posix/sys/statfs.h>
#include <Prism/Memory/Buffer.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

module Cryptix:Impl.VFS.DevTmpFs.DevTmpFs;

import :System.Limits;
import :Time.Time;
import :VFS.DevTmpFs.DevTmpFs;
import :VFS.DevTmpFs.DevTmpFsINode;
import :VFS.INode;
import :VFS.VFS;

DevTmpFs::DevTmpFs(u32 flags)
    : Filesystem("DevTmpFs", flags)
{
}

ErrorOr<::Ref<DirectoryEntry>> DevTmpFs::Mount(StringView  sourcePath,
                                               const void* data)
{
    m_BlockSize          = PMM::PAGE_SIZE;
    m_BytesLimit         = PMM::GetTotalMemory() / 2;

    m_MaxBlockCount      = 64_kib;
    m_UsedBlockCount     = 0;

    m_MaxINodeCount      = PMM::GetTotalMemory() / PMM::PAGE_SIZE / 2;
    m_FreeINodeCount     = m_MaxINodeCount;

    m_MaxSize            = PMM::GetTotalMemory() / 2;

    m_RootEntry          = CreateRef<DirectoryEntry>(nullptr, "/");
    m_Root               = TryOrRet(AllocateNode("/", 0644 | S_IFDIR));

    auto inode           = m_Root.As<DevTmpFsINode>();
    inode->m_Metadata.ID = 2;

    m_RootEntry->Bind(m_Root);
    m_RootEntry->SetParent(m_RootEntry);

    return m_RootEntry;
}

ErrorOr<::Ref<INode>> DevTmpFs::AllocateNode(StringView name, INodeMode mode)
{
    if (m_NextINodeIndex >= m_MaxINodeCount) return Error(ENOSPC);
    else if (m_FreeINodeCount == 0) return Error(ENOSPC);

    auto inode = CreateRef<DevTmpFsINode>(name, this, NextINodeIndex(), mode);
    if (!inode) return Error(ENOMEM);

    --m_FreeINodeCount;
    return inode;
}
ErrorOr<void> DevTmpFs::FreeINode(::Ref<INode> inode)
{
    if (!inode) return Error(EFAULT);

    inode.Reset();
    ++m_FreeINodeCount;

    return {};
}

ErrorOr<void> DevTmpFs::Stats(statfs& stats)
{
    using namespace System;
    Memory::Fill(&stats, 0, sizeof(statfs));

    stats.f_type   = TMPFS_MAGIC;
    stats.f_bsize  = PMM::PAGE_SIZE;
    stats.f_blocks = m_MaxBlockCount;
    stats.f_bfree = stats.f_bavail = m_MaxBlockCount - m_UsedBlockCount;

    stats.f_files                  = m_MaxINodeCount;
    stats.f_ffree                  = m_FreeINodeCount;
    stats.f_fsid                   = m_ID;
    stats.f_namelen                = Limits::FILE_NAME;
    stats.f_frsize                 = 0;
    stats.f_flags                  = m_Flags;

    return {};
}
