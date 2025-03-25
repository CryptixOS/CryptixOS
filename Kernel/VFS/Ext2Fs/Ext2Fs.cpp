/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Time/Time.hpp>

#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Ext2Fs/Ext2FsINode.hpp>

INode* Ext2Fs::Mount(INode* parent, INode* source, INode* target,
                     std::string_view name, void* data)
{
    m_Device     = source;

    m_SuperBlock = new Ext2FsSuperBlock;
    m_Device->Read(m_SuperBlock, 1024, sizeof(Ext2FsSuperBlock));

    constexpr usize EXT2_FS_SIGNATURE = 0xef53;
    if (m_SuperBlock->Signature != EXT2_FS_SIGNATURE)
    {
        LogError("Ext2Fs: '{}' -> Invalid signature", source->GetPath());
        delete m_SuperBlock;
        return nullptr;
    }

    if (m_SuperBlock->VersionMajor < 1)
    {
        LogError("Ext2Fs: '{}' -> Unsupported ext2fs version: {}",
                 source->GetPath(), m_SuperBlock->VersionMajor);
        delete m_SuperBlock;
        return nullptr;
    }

    m_BlockSize    = 1024 << m_SuperBlock->BlockSize;
    m_FragmentSize = 1024 << m_SuperBlock->FragmentSize;
    m_BlockGroupDescriptionCount
        = m_SuperBlock->BlockCount / m_SuperBlock->BlocksPerGroup;

    m_SuperBlock->LastMountTime = Time::GetReal().tv_sec;
    m_Device->Write(m_SuperBlock, 1024, sizeof(Ext2FsSuperBlock));

    auto* root = new Ext2FsINode(parent, name, this, 0644 | S_IFDIR);
    ReadINodeEntry(&root->m_Meta, 2);

    root->m_Stats.st_ino   = 2;
    root->m_Stats.st_dev   = source->GetStats().st_rdev;
    root->m_Stats.st_nlink = root->m_Meta.HardLinkCount;
    root->m_Stats.st_size  = root->m_Meta.SizeLow
                          | (static_cast<u64>(root->m_Meta.SizeHigh) >> 32);
    root->m_Stats.st_blocks = root->m_Stats.st_size / root->m_Stats.st_blksize;

    if (!root)
    {
        LogError("Ext2Fs: '{}' -> Failed to create root node",
                 source->GetPath());
        delete m_SuperBlock;
        return nullptr;
    }

    m_MountedOn = target;
    return (m_Root = root);
}

INode* Ext2Fs::CreateNode(INode* parent, std::string_view name, mode_t mode)
{
    return nullptr;
}

bool Ext2Fs::Populate(INode* node)
{
    Ext2FsINode*    e2node = reinterpret_cast<Ext2FsINode*>(node);
    Ext2FsINodeMeta parentMeta;
    ReadINodeEntry(&parentMeta, e2node->m_Stats.st_ino);

    u8* buffer = new u8[parentMeta.GetSize()];
    ReadINode(parentMeta, buffer, 0, parentMeta.GetSize());

    for (usize i = 0; i < parentMeta.GetSize();)
    {
        Ext2FsDirectoryEntry* entry
            = reinterpret_cast<Ext2FsDirectoryEntry*>(buffer + i);

        char* nameBuffer = new char[entry->NameSize + 1];
        std::strncpy(nameBuffer, reinterpret_cast<char*>(entry->Name),
                     entry->NameSize);
        nameBuffer[entry->NameSize] = 0;

        if (entry->INodeIndex == 0)
        {
            delete[] nameBuffer;
            i += entry->Size;

            continue;
        }
        if (!std::strcmp(nameBuffer, ".") || !std::strcmp(nameBuffer, ".."))
        {
            i += entry->Size;
            continue;
        }

        Ext2FsINodeMeta inodeMeta;
        ReadINodeEntry(&inodeMeta, entry->INodeIndex);

        u64 mode = (inodeMeta.Permissions & 0xfff);
        switch (entry->Type)
        {
            case Ext2FsDirectoryEntryType::eRegular: mode |= S_IFREG; break;
            case Ext2FsDirectoryEntryType::eDirectory: mode |= S_IFDIR; break;
            case Ext2FsDirectoryEntryType::eCharacterDevice:
                mode |= S_IFCHR;
                break;
            case Ext2FsDirectoryEntryType::eBlockDevice: mode |= S_IFBLK; break;
            case Ext2FsDirectoryEntryType::eFifo: mode |= S_IFIFO; break;
            case Ext2FsDirectoryEntryType::eSocket: mode |= S_IFSOCK; break;
            case Ext2FsDirectoryEntryType::eSymlink: mode |= S_IFLNK; break;

            default:
                LogError("Ext2Fs: Invalid directory entry type: {}",
                         std::to_underlying(entry->Type));
                break;
        }

        Ext2FsINode* newNode    = new Ext2FsINode(node, nameBuffer, this, mode);
        newNode->m_Stats.st_uid = inodeMeta.UID;
        newNode->m_Stats.st_gid = inodeMeta.GID;
        newNode->m_Stats.st_ino = entry->INodeIndex;
        newNode->m_Stats.st_size   = inodeMeta.GetSize();
        newNode->m_Stats.st_nlink  = inodeMeta.HardLinkCount;
        newNode->m_Stats.st_blocks = newNode->m_Stats.st_size / m_BlockSize;

        newNode->m_Stats.st_atim.tv_sec  = inodeMeta.AccessTime;
        newNode->m_Stats.st_atim.tv_nsec = 0;
        newNode->m_Stats.st_ctim.tv_sec  = inodeMeta.CreationTime;
        newNode->m_Stats.st_ctim.tv_nsec = 0;
        newNode->m_Stats.st_mtim.tv_sec  = inodeMeta.ModifiedTime;
        newNode->m_Stats.st_mtim.tv_nsec = 0;

        newNode->m_Populated             = false;
        newNode->m_Meta                  = inodeMeta;
        e2node->InsertChild(newNode, newNode->GetName());

        // TODO(v1tr10l7): resolve link
        if (newNode->IsSymlink())
            ;

        delete[] nameBuffer;
        i += entry->Size;
    }

    e2node->m_Populated = true;
    delete[] buffer;
    return e2node->m_Populated;
}

void Ext2Fs::ReadINodeEntry(Ext2FsINodeMeta* out, u32 index)
{
    usize tableIndex      = (index - 1) % m_SuperBlock->INodesPerGroup;
    usize blockGroupIndex = (index - 1) / m_SuperBlock->INodesPerGroup;

    Ext2FsBlockGroupDescriptor blockGroup;
    ReadBlockGroupDescriptor(&blockGroup, blockGroupIndex);

    m_Device->Read(out,
                   blockGroup.INodeTableAddress * m_BlockSize
                       + m_SuperBlock->INodeStructureSize * tableIndex,
                   sizeof(Ext2FsINodeMeta));
}
void Ext2Fs::WriteINodeEntry(Ext2FsINodeMeta& in, u32 index)
{
    usize tableIndex      = (index - 1) % m_SuperBlock->INodesPerGroup;
    usize blockGroupIndex = (index - 1) / m_SuperBlock->INodesPerGroup;

    Ext2FsBlockGroupDescriptor blockGroup;
    ReadBlockGroupDescriptor(&blockGroup, blockGroupIndex);

    m_Device->Write(&in,
                    blockGroup.INodeTableAddress * m_BlockSize
                        + m_SuperBlock->INodeStructureSize * tableIndex,
                    sizeof(Ext2FsINodeMeta));
}

isize Ext2Fs::ReadINode(Ext2FsINodeMeta& meta, u8* out, off_t offset,
                        usize bytes)
{
    if (static_cast<usize>(offset) > meta.GetSize()) return 0;
    if (static_cast<usize>(offset) + bytes > meta.GetSize())
        bytes = meta.GetSize() - offset;

    for (usize head = 0; head < bytes;)
    {
        usize blockIndex = (offset + head) / m_BlockSize;

        usize size       = bytes - head;
        offset           = (offset + head) % m_BlockSize;

        if (size > (m_BlockSize - offset)) size = m_BlockSize - offset;

        u32 block = GetINodeBlock(meta, blockIndex);
        m_Device->Read(out + head, block * m_BlockSize + offset, size);

        head += size;
    }

    return bytes;
}

void Ext2Fs::ReadBlockGroupDescriptor(Ext2FsBlockGroupDescriptor* out,
                                      usize                       index)
{
    off_t offset = m_BlockSize >= 2048 ? m_BlockSize : m_BlockSize * 2;

    m_Device->Read(out, offset + sizeof(Ext2FsBlockGroupDescriptor) * index,
                   sizeof(Ext2FsBlockGroupDescriptor));
}
u32 Ext2Fs::GetINodeBlock(Ext2FsINodeMeta& meta, u32 blockIndex)
{
    u32 block      = 0;
    u32 blockLevel = m_BlockSize / 4;

    if (blockIndex < 12)
    {
        block = meta.Blocks[blockIndex];
        return block;
    }

    blockIndex -= 12;

    if (blockIndex >= blockLevel)
    {
        blockIndex -= blockLevel;

        u32   singleIndex    = blockIndex / blockLevel;
        off_t indirectOffset = blockIndex % blockLevel;
        u32   indirectBlock  = 0;

        if (singleIndex >= blockLevel)
        {
            blockIndex -= blockLevel * blockLevel;

            u32 doubleIndirect      = blockIndex / blockLevel;
            indirectOffset          = blockIndex % blockLevel;
            u32 singleIndirectIndex = 0;
            m_Device->Read(&singleIndirectIndex,
                           meta.Blocks[14] * m_BlockSize + doubleIndirect * 4,
                           sizeof(u32));
            m_Device->Read(&indirectBlock,
                           doubleIndirect * m_BlockSize
                               + singleIndirectIndex * 4,
                           sizeof(u32));
            m_Device->Read(&block,
                           indirectBlock * m_BlockSize + indirectOffset * 4,
                           sizeof(u32));

            return block;
        }

        m_Device->Read(&indirectBlock,
                       meta.Blocks[13] * m_BlockSize + singleIndex * 4,
                       sizeof(u32));
        m_Device->Read(&block, indirectBlock * m_BlockSize + indirectOffset * 4,
                       sizeof(u32));

        return block;
    }

    m_Device->Read(&block, meta.Blocks[12] * m_BlockSize + blockIndex * 4,
                   sizeof(u32));
    return block;
}
