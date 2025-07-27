/*
 * Created by v1tr10l7 on 23.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/Fat32Fs/Fat32FsINode.hpp>
#include <VFS/Fat32Fs/Fat32FsStructures.hpp>
#include <VFS/Filesystem.hpp>

class Fat32Fs final : public Filesystem
{
  public:
    explicit Fat32Fs(u32 flags)
        : Filesystem("Fat32Fs", flags)
    {
    }
    virtual ~Fat32Fs() = default;

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;
    ErrorOr<INode*>       CreateNode(INode* parent, ::Ref<DirectoryEntry> entry,
                                     mode_t mode, uid_t uid = 0, gid_t gid = 0);
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
    INode*          m_Device = nullptr;
    Fat32BootRecord m_BootRecord;
    Fat32FsInfo     m_FsInfo;
    usize           m_ClusterSize    = 0;
    usize           m_ClusterCount   = 0;
    isize           m_FatOffset      = 0;
    isize           m_DataOffset     = 0;
    Atomic<i64>     m_NextINodeIndex = 3;
    Fat32FsINode*   m_RootNode       = nullptr;

    usize           GetClusterOffset(u32 cluster);
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
