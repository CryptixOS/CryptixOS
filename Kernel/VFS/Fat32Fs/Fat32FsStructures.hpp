/*
 * Created by v1tr10l7 on 25.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

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

enum class Fat32FsVersion
{
    eFat12,
    eFat16,
    eFat32
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
static_assert(sizeof(Fat32DirectoryEntry) == 32);

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
static_assert(sizeof(Fat32LfnDirectoryEntry) == 32);
