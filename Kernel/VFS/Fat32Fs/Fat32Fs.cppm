/*
 * Created by v1tr10l7 on 23.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

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
#include <Prism/Core/TypeTraits.hpp>
#include <API/Posix/sys/statfs.h>

export module Cryptix:VFS.Fat32Fs.Fat32Fs;

export import :VFS.Fat32Fs.Fat32FsINode;
export import :VFS.Fat32Fs.Fat32FsStructures;
export import :VFS.Filesystem;

export class Fat32Fs final : public Filesystem
{
  public:
    explicit Fat32Fs(u32 flags);
    virtual ~Fat32Fs() = default;

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;
    ErrorOr<::Ref<INode>> CreateNode(::Ref<INode>          parent,
                                     ::Ref<DirectoryEntry> entry, mode_t mode,
                                     uid_t uid = 0, gid_t gid = 0);
    virtual bool          Populate(DirectoryEntry* dentry) override;

    virtual ErrorOr<void> Stats(statfs& stats) override;

    constexpr bool        IsFinalCluster(usize cluster) const
    {
        return cluster >= 0xffffff8;
    }

    bool  ReadBytes(u32 cluster, u8* out, off_t offset, usize bytes);
    usize GetChainSize(u32 cluster);

    u32   AllocateCluster();
    u32   AllocateClusters(usize count);
    u32   GetNextCluster(u32 cluster);
    u32   SkipCluster(u32 cluster, usize count, bool& endCluster);

  private:
    ::Ref<INode>        m_Device = nullptr;
    Fat32BootRecord     m_BootRecord;
    Fat32FsInfo         m_FsInfo;
    usize               m_ClusterSize    = 0;
    usize               m_ClusterCount   = 0;
    isize               m_FatOffset      = 0;
    isize               m_DataOffset     = 0;
    Atomic<i64>         m_NextINodeIndex = 3;
    ::Ref<Fat32FsINode> m_RootNode       = nullptr;

    usize               GetClusterOffset(u32 cluster);
    constexpr usize
    GetClusterForDirectoryEntry(Fat32DirectoryEntry* entry) const
    {
        return (static_cast<u32>(entry->ClusterLow)
                | (static_cast<u32>(entry->ClusterHigh) << 16));
    }
    isize ReadWriteCluster(u8* dest, u32 cluster, isize offset, usize bytes,
                           bool write = false);
    isize ReadWriteClusters(u8* dest, u32 cluster, usize count, u32* endCluster,
                            bool write = false);

    usize CountSpacePadding(u8* str, usize len);
    u8    GetLfnChecksum(u8* shortName);
    void  CopyLfnToString(Fat32LfnDirectoryEntry* entry, char* str);
    void  UpdateFsInfo();
};
