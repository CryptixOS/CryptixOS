/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

enum class Ext2FsState : u16
{
    eClean     = 0x01,
    eHasErrors = 0x02,
};
enum class Ext2FsOnError : u16
{
    eIgnore          = 0x01,
    eRemountReadOnly = 0x02,
    eKernelPanic     = 0x03,
};

struct [[gnu::packed]] Ext2FsSuperBlock
{
    u32           INodeCount;
    u32           BlockCount;
    u32           SuperUserReservedBlockCount;
    u32           FreeBlockCount;
    u32           FreeINodeCount;
    u32           SuperBlockNumber;
    u32           BlockSize;
    u32           FragmentSize;
    u32           BlocksPerGroup;
    u32           FragmentsPerGroup;
    u32           INodesPerGroup;
    u32           LastMountTime;
    u32           LastWrittenTime;
    u16           MountedSinceFsckCount;
    u16           MountsAllowedBeforeFsck;
    u16           Signature;
    Ext2FsState   FilesystemState;
    Ext2FsOnError OnErrorDetected;
    u16           VersionMinor;
    u32           LastFsckTime;
    u32           FsckInterval;
    u32           OperatingSystemID;
    u32           VersionMajor;
    u16           UID;
    u16           GID;

    u32           FirstNonReservedINode;
    u16           INodeStructureSize;
    u16           SuperBlockGroup;
    u32           OptionalFeatures;
    u32           RequiredFeatures;
    u32           FeaturesRequiredForNotReadOnly;
    u8            FilesystemUUID[16];
    u8            VolumeName[16];
    u8            LastMountPath[64];
    u32           CompressionAlgorithmsUsed;
    u8            PreallocatedBlocksPerFile;
    u8            PreallocatedBlocksPerDirectory;
    u16           Reserved;
    u8            JournalID[16];
    u32           JournalINode;
    u32           JournalDevice;
    u32           HeadOfOrphanINodeList;
};

struct [[gnu::packed]] Ext2FsBlockGroupDescriptor
{
    u32 BlockUsageBitmapAddress;
    u32 INodeUsageBitmapAddress;
    u32 INodeTableAddress;
    u16 FreeBlockCount;
    u16 FreeINodeCount;
    u16 DirectoryCount;
    u16 Reserved[7];
};
struct [[gnu::packed]] Ext2FsINodeMeta
{
    u16             Permissions;
    u16             UID;
    u32             SizeLow;
    u32             AccessTime;
    u32             CreationTime;
    u32             ModifiedTime;
    u32             DeletedTime;
    u16             GID;
    u16             HardLinkCount;
    u32             SectorCount;
    u32             Flags;
    u32             OperatingSystemSpecific1;
    u32             Blocks[15];
    u32             GenerationNumber;
    u32             ExtendedAttributeBlock;
    u32             SizeHigh;
    u32             FragmentAddress;
    u32             OperatingSystemSpecific2[3];

    constexpr usize GetSize() const
    {
        return static_cast<usize>(SizeLow)
             | (static_cast<usize>(SizeHigh) << 32);
    }
    inline void SetSize(usize sizeLow, usize sizeHigh)
    {
        SizeLow  = sizeLow;
        SizeHigh = sizeHigh >> 32;
    }
};

enum class Ext2FsDirectoryEntryType : u8
{
    eUnknown         = 0x00,
    eRegular         = 0x01,
    eDirectory       = 0x02,
    eCharacterDevice = 0x03,
    eBlockDevice     = 0x04,
    eFifo            = 0x05,
    eSocket          = 0x06,
    eSymlink         = 0x07,
};
struct [[gnu::packed]] Ext2FsDirectoryEntry
{
    u32                      INodeIndex;
    u16                      Size;
    u8                       NameSize;
    Ext2FsDirectoryEntryType Type;
    u8                       Name[];
};
