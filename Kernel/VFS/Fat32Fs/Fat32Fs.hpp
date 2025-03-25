/*
 * Created by v1tr10l7 on 23.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/Fat32Fs/Fat32FsINode.hpp>
#include <VFS/Filesystem.hpp>

struct [[gnu::packed]] Fat32BootRecord
{
    u8  Jump[3];
    u8  Oem[8];
    u16 BytesPerSector;
    u8  SectorsPerCluster;
    u16 ReservedSectorCount;
    u8  FatCount;
    u16 RootDirEntryCount;
    u16 SectorCount;
    u16 SectorPerFatUnused;
    u8  MediaDescriptor;
    u32 Geometry;
    u32 HiddenSectors;
    u32 LargeSectorCount;
    u32 SectorsPerFat;
    u16 Flags;
    u16 Version;
    u32 RootDirectoryCluster;
    u16 FsInfoSector;
    u16 BackupBootSector;
    u32 Reserved[3];
    u8  Drive;
    u8  FlagNt;
    u8  Signature;
    u32 VolumeID;
    u8  VolumeLabel[11];
    u8  IdentifierString[8];
};
struct [[gnu::packed]] Fat32FsInfo
{
    u32 Signature;
    u32 Free;
    u32 StartAt;
    u32 Reserved[3];
    u32 Signature2;
};

enum class Fat32Attribute : u8
{
    eReadOnly     = 0x01,
    eHidden       = 0x02,
    eSystem       = 0x04,
    eVolumeID     = 0x08,
    eDirectory    = 0x10,
    eArchive      = 0x20,
    eDevice       = 0x40,
    eLongFileName = eVolumeID | eSystem | eHidden | eReadOnly,
};
constexpr bool operator&(Fat32Attribute lhs, Fat32Attribute rhs)
{
    return std::to_underlying(lhs) & std::to_underlying(rhs);
}

struct [[gnu::packed]] Fat32DirectoryEntry
{
    u8             Name[11];
    Fat32Attribute Attributes;
    u8             Reserved;
    u8             CreateTimeTenth;
    u16            CreationTime;
    u16            CreationDate;
    u16            AccessDate;
    u16            ClusterHigh;
    u16            LastModTime;
    u16            LastModDate;
    u16            ClusterLow;
    u32            Size;
};
struct [[gnu::packed]] Fat32LfnDirectoryEntry
{
    u8  Order;
    u16 Name1[5];
    u8  Attribute;
    u8  Reserved;
    u8  Checksum;
    u16 Name2[6];
    u16 Zero;
    u16 Name3[2];
};

class Fat32Fs final : public Filesystem
{
  public:
    Fat32Fs(u32 flags)
        : Filesystem("Fat32Fs", flags)
    {
    }
    virtual ~Fat32Fs() = default;

    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name,
                         const void*      data = nullptr) override;
    virtual INode* CreateNode(INode* parent, std::string_view name,
                              mode_t mode) override;
    virtual INode* Symlink(INode* parent, std::string_view name,
                           std::string_view target) override
    {
        return nullptr;
    }

    virtual INode* Link(INode* parent, std::string_view name,
                        INode* oldNode) override
    {
        return nullptr;
    }
    virtual bool   Populate(INode* node) override;

    constexpr bool IsFinalCluster(usize cluster) const
    {
        return cluster >= 0xffffff8;
    }
    usize GetChainSize(u32 cluster);
    u32   GetNextCluster(u32 cluster);

  private:
    INode*           m_Device = nullptr;
    Fat32BootRecord  m_BootRecord;
    Fat32FsInfo      m_FsInfo;
    usize            m_ClusterSize    = 0;
    usize            m_ClusterCount   = 0;
    isize            m_FatOffset      = 0;
    isize            m_DataOffset     = 0;
    std::atomic<i64> m_NextINodeIndex = 3;
    Fat32FsINode*    m_RootNode       = nullptr;

    usize            GetClusterOffset(u32 cluster);
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
