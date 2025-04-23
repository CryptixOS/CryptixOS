/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/PMM.hpp>

#include <Prism/Utility/Math.hpp>

#include <VFS/INode.hpp>

#include <VFS/Fat32Fs/Fat32Fs.hpp>
#include <VFS/Fat32Fs/Fat32FsINode.hpp>

#include <Time/Time.hpp>

#include <cctype>

constexpr const char* FAT32_IDENTIFIER_STRING       = "FAT32   ";
constexpr usize       FAT32_FS_INFO_SIGNATURE       = 0x41615252;
constexpr usize       FAT32_FS_INFO_OFFSET          = 484;
constexpr usize       FAT32_REAL_FS_INFO_SIGNATURE  = 0x61417272;
constexpr usize       FAT32_REAL_FS_INFO_SIGNATURE2 = 0xaa550000;

INode* Fat32Fs::Mount(INode* parent, INode* source, INode* target,
                      StringView name, const void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    source->Read(&m_BootRecord, 0, sizeof(Fat32BootRecord));
    if (m_BootRecord.Signature != 0x29)
    {
        LogError("Fat32: '{}' -> Bad boot record signature", source->GetPath());
        return nullptr;
    }

    if (std::strncmp(reinterpret_cast<char*>(m_BootRecord.IdentifierString),
                     FAT32_IDENTIFIER_STRING, 8))
    {
        LogError("Fat32: '{}' -> Bad identifier string", source->GetPath());
        return nullptr;
    }

    source->Read(&m_FsInfo,
                 m_BootRecord.FsInfoSector * m_BootRecord.BytesPerSector,
                 sizeof(Fat32FsInfo));
    if (m_FsInfo.Signature != FAT32_FS_INFO_SIGNATURE)
    {
        LogError("Fat32: '{}' -> bad fsinfo signature", source->GetPath());
        return nullptr;
    }

    source->Read(&m_FsInfo,
                 m_BootRecord.FsInfoSector * m_BootRecord.BytesPerSector
                     + FAT32_FS_INFO_OFFSET,
                 sizeof(Fat32FsInfo));

    if (m_FsInfo.Signature != FAT32_REAL_FS_INFO_SIGNATURE
        || m_FsInfo.Signature2 != FAT32_REAL_FS_INFO_SIGNATURE2)
    {
        LogError("Fat32: '{}' -> Bad fsinfo signature", source->GetPath());
        return nullptr;
    }

    off_t dataSector = m_BootRecord.ReservedSectorCount
                     + m_BootRecord.FatCount * m_BootRecord.SectorsPerFat;

    m_Device = source;
    m_ClusterSize
        = m_BootRecord.SectorsPerCluster * m_BootRecord.BytesPerSector;
    m_FatOffset
        = m_BootRecord.ReservedSectorCount * m_BootRecord.BytesPerSector;
    m_DataOffset   = dataSector * m_BootRecord.BytesPerSector;
    m_ClusterCount = (m_Device->GetStats().st_blocks - dataSector)
                   / m_BootRecord.SectorsPerCluster;

    UpdateFsInfo();

    m_Root     = CreateNode(parent, name, 0644 | S_IFDIR);
    m_RootNode = reinterpret_cast<Fat32FsINode*>(m_Root);

    m_RootNode->m_Stats.st_blocks
        = GetChainSize(m_BootRecord.RootDirectoryCluster);
    m_RootNode->m_Stats.st_size = m_RootNode->m_Stats.st_blocks * m_ClusterSize;
    m_RootNode->m_Stats.st_blksize = m_ClusterSize;
    m_RootNode->m_Stats.st_dev     = m_Device->GetStats().st_rdev;

    m_RootNode->m_Cluster          = m_BootRecord.RootDirectoryCluster;

    if (m_RootNode) m_MountedOn = target;
    m_Root = m_RootNode;
    return m_RootNode;
}

INode* Fat32Fs::CreateNode(INode* parent, StringView name, mode_t mode)
{
    if (name.Size() > 255) return_err(nullptr, ENAMETOOLONG);
    if (!S_ISREG(mode) && !S_ISDIR(mode)) return_err(nullptr, EPERM);

    return new Fat32FsINode(parent, name, this, mode);
}

bool Fat32Fs::Populate(INode* node)
{
    String nameBuffer;
    nameBuffer.Resize(256);

    Fat32DirectoryEntry* directoryEntries
        = reinterpret_cast<Fat32DirectoryEntry*>(
            new u8[node->GetStats().st_size]);
    if (!directoryEntries)
    {
        delete[] directoryEntries;
        return false;
    }

    usize directoryEntryCount
        = node->GetStats().st_size / sizeof(Fat32DirectoryEntry);

    Fat32FsINode* f32node = reinterpret_cast<Fat32FsINode*>(node);
    if (ReadWriteClusters(reinterpret_cast<u8*>(directoryEntries),
                          f32node->m_Cluster, f32node->m_Stats.st_blocks,
                          nullptr, false)
        == -1)
    {
        LogError("Fat32::Populate: Failed to read/write clusters");
        delete[] directoryEntries;
        return false;
    }

    bool isLfn = false;
    u8   checksum;
    for (usize i = 0; i < directoryEntryCount; ++i)
    {
        Fat32DirectoryEntry* entry = directoryEntries + i;
        if (entry->Name[0] == 0) break;
        else if (entry->Name[0] == 0xe5) continue;

        if (entry->Attributes & Fat32Attribute::eLongFileName)
        {
            Fat32LfnDirectoryEntry* lfn
                = reinterpret_cast<Fat32LfnDirectoryEntry*>(entry);
            lfn->Order &= 0x1f;

            if (!isLfn)
            {
                Fat32DirectoryEntry* directoryEntry = entry + lfn->Order;
                checksum = GetLfnChecksum(directoryEntry->Name);
                isLfn    = true;
            }
            else if (lfn->Checksum != checksum)
            {
                LogError("Fat32: Bad lfn checksum. Aborting populate");
                delete[] directoryEntries;
                return false;
            }

            CopyLfnToString(lfn, nameBuffer.Raw() + (lfn->Order - 1) * 13);
            continue;
        }
        if (entry->Attributes & Fat32Attribute::eVolumeID) continue;

        if (!isLfn)
        {
            usize nameSize = 8 - CountSpacePadding(entry->Name, 8);
            usize extSize  = 3 - CountSpacePadding(entry->Name + 8, 3);
            if (nameSize)
                std::strncpy(nameBuffer.Raw(),
                             reinterpret_cast<char*>(entry->Name), nameSize);
            if (extSize)
            {
                nameBuffer[nameSize] = '.';
                std::strncpy(nameBuffer.Raw() + nameSize + 1,
                             reinterpret_cast<char*>(entry->Name) + 8, extSize);
            }
        }

        isLfn = false;
        if (std::strcmp(nameBuffer.Raw(), ".") == 0
            || std::strcmp(nameBuffer.Raw(), "..") == 0)
            continue;

        u16 mode = 0644
                 | (entry->Attributes & Fat32Attribute::eDirectory ? S_IFDIR
                                                                   : S_IFREG);
        LogInfo("Directory: {}, RegularFile: {}",
                entry->Attributes & Fat32Attribute::eDirectory,
                entry->Attributes & Fat32Attribute::eSystem);

        Fat32FsINode* newNode = reinterpret_cast<Fat32FsINode*>(
            CreateNode(node, nameBuffer, mode));
        if (!newNode)
        {
            LogError("Fat32::Populate: Failed to create new node");
            delete[] directoryEntries;
            return false;
        }

        // TODO(v1tr10l7): atime, ctime, mtime

        newNode->m_Cluster          = GetClusterForDirectoryEntry(entry);
        newNode->m_Stats.st_blksize = m_ClusterSize;
        newNode->m_Stats.st_size
            = S_ISDIR(mode) ? GetChainSize(newNode->m_Cluster) * m_ClusterSize
                            : entry->Size;
        newNode->m_Stats.st_blocks
            = Math::DivRoundUp(newNode->m_Stats.st_size, m_ClusterSize);
        newNode->m_DirectoryOffset
            = reinterpret_cast<uintptr_t>(entry)
            - reinterpret_cast<uintptr_t>(directoryEntries);

        usize nameLen = 0;
        char* start   = nameBuffer.Raw();
        while (*start && std::isalnum(*start++)) nameLen++;
        String name;
        name.Resize(nameLen);

        nameBuffer.Copy(name.Raw(), nameLen);
        node->InsertChild(newNode, newNode->GetName());

        if (S_ISDIR(mode)) newNode->m_Populated = false;
    }

    delete[] directoryEntries;
    return (f32node->m_Populated = true);
}

bool Fat32Fs::ReadBytes(u32 cluster, u8* out, off_t offset, usize bytes)
{
    bool end            = true;
    cluster             = SkipCluster(cluster, offset / m_ClusterSize, end);

    off_t clusterOffset = offset % m_ClusterSize;
    if (clusterOffset > 0)
    {
        usize count
            = bytes > m_ClusterSize ? m_ClusterSize - clusterOffset : bytes;
        if (ReadWriteCluster(out, cluster, clusterOffset, count, false) == -1)
            return false;

        cluster = GetNextCluster(cluster);
        out += count;
        bytes -= count;
    }

    usize clustersRemaining = bytes / m_ClusterSize;
    if (clustersRemaining
        && ReadWriteClusters(out, cluster, clustersRemaining, &cluster, false)
               == -1)
        return false;

    bytes -= clustersRemaining * m_ClusterSize;
    if (bytes > 0)
    {
        out += clustersRemaining * m_ClusterSize;
        if (ReadWriteCluster(out, cluster, 0, bytes, false) == -1) return false;
    }

    return true;
}

usize Fat32Fs::GetChainSize(u32 cluster)
{
    usize count = 0;
    while (!IsFinalCluster(cluster))
    {
        ++count;
        cluster = GetNextCluster(cluster);
    }

    return count;
}
u32 Fat32Fs::GetNextCluster(u32 cluster)
{
    m_Device->Read(&cluster, m_FatOffset + cluster * 4, 4);
    return cluster;
}
u32 Fat32Fs::SkipCluster(u32 cluster, usize count, bool& end)
{
    end = false;
    for (usize i = 0; i < count; ++i)
    {
        u32 next = GetNextCluster(cluster);
        if (IsFinalCluster(next))
        {
            end = true;
            break;
        }

        cluster = next;
    }

    return cluster;
}

usize Fat32Fs::GetClusterOffset(u32 cluster)
{
    return m_DataOffset + m_ClusterSize * (cluster - 2);
}
isize Fat32Fs::ReadWriteCluster(u8* dest, u32 cluster, isize offset,
                                usize bytes, bool write)
{
    return write
             ? m_Device->Write(dest, GetClusterOffset(cluster) + offset, bytes)
             : m_Device->Read(dest, GetClusterOffset(cluster) + offset, bytes);
}
isize Fat32Fs::ReadWriteClusters(u8* dest, u32 cluster, usize count,
                                 u32* endCluster, bool write)
{
    usize i = 0;
    for (; i < count; ++i)
    {
        AssertMsg(cluster, "Fat32: Tried to read/write free cluster");
        if (IsFinalCluster(cluster)) break;
        Assert(cluster < m_ClusterCount);

        isize diskOffset = GetClusterOffset(cluster);
        isize status     = write ? m_Device->Write(dest + m_ClusterSize * i,
                                                   diskOffset, m_ClusterSize)
                                 : m_Device->Read(dest + m_ClusterSize * i,
                                                  diskOffset, m_ClusterSize);
        if (status == -1) return -1;

        cluster = GetNextCluster(cluster);
    }

    if (endCluster) *endCluster = cluster;
    return i;
}

usize Fat32Fs::CountSpacePadding(u8* str, usize len)
{
    usize count = 0;
    str += len - 1;
    while (len-- && *str-- == 0x20) count++;

    return count;
}
u8 Fat32Fs::GetLfnChecksum(u8* shortName)
{
    u8 checksum = 0;
    for (i32 i = 11; i; --i)
        checksum = ((checksum & 1) << 7) + (checksum >> 1) + *shortName++;

    return checksum;
}
void Fat32Fs::CopyLfnToString(Fat32LfnDirectoryEntry* entry, char* str)
{
    for (i32 i = 0; i < 5; ++i) *str++ = entry->Name1[i];
    for (i32 i = 0; i < 6; ++i) *str++ = entry->Name2[i];
    for (i32 i = 0; i < 2; ++i) *str++ = entry->Name3[i];
}
void Fat32Fs::UpdateFsInfo()
{
    if (m_FsInfo.Free == 0xffffffff || m_FsInfo.StartAt == 0xffffffff)
    {
        u32* fatBuffer
            = reinterpret_cast<u32*>(new u8[m_BootRecord.BytesPerSector]);

        usize clustersPerFatSector = m_BootRecord.BytesPerSector / 4;
        m_FsInfo.Free              = 0;

        for (usize sector = 0; sector < m_BootRecord.SectorsPerFat; ++sector)
        {
            Assert(m_Device->Read(fatBuffer,
                                  m_FatOffset
                                      + sector * m_BootRecord.BytesPerSector,
                                  m_BootRecord.BytesPerSector)
                   >= 0);
            for (usize cluster = 0; cluster < clustersPerFatSector; ++cluster)
            {
                if (fatBuffer[cluster] == 0)
                {
                    if (m_FsInfo.StartAt == 0xffffffff)
                        m_FsInfo.StartAt
                            = cluster + sector * clustersPerFatSector;
                }

                ++m_FsInfo.Free;
            }
        }

        delete fatBuffer;
    }

    m_Device->Write(&m_FsInfo,
                    m_BootRecord.BytesPerSector * m_BootRecord.FsInfoSector
                        + FAT32_FS_INFO_OFFSET,
                    sizeof(Fat32FsInfo));
}
