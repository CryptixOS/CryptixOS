/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Containers/Bitmap.hpp>
#include <Time/Time.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Ext2Fs/Ext2FsINode.hpp>

ErrorOr<::Ref<DirectoryEntry>> Ext2Fs::Mount(StringView  sourcePath,
                                             const void* data)
{
    auto sourceEntry = VFS::ResolvePath(nullptr, sourcePath)
                           .ValueOr(VFS::PathResolution{})
                           .Entry;
    if (!sourceEntry || !sourceEntry->INode()) return Error(ENODEV);

    m_Device     = sourceEntry->INode();

    m_SuperBlock = new Ext2FsSuperBlock;
    ReadSuperBlock();

    constexpr usize EXT2_FS_SIGNATURE = 0xef53;
    if (m_SuperBlock->Signature != EXT2_FS_SIGNATURE)
    {
        LogError("Ext2Fs: '{}' -> Invalid signature", sourceEntry->Path());
        delete m_SuperBlock;
        return nullptr;
    }

    if (m_SuperBlock->VersionMajor < 1)
    {
        LogError("Ext2Fs: '{}' -> Unsupported ext2fs version: {}",
                 sourceEntry->Path(), m_SuperBlock->VersionMajor);
        delete m_SuperBlock;
        return nullptr;
    }

    m_BlockSize    = 1024 << m_SuperBlock->BlockSize;
    m_FragmentSize = 1024 << m_SuperBlock->FragmentSize;
    m_BlockGroupDescriptionCount
        = m_SuperBlock->BlockCount / m_SuperBlock->BlocksPerGroup;

    m_SuperBlock->LastMountTime = Time::GetReal().tv_sec;
    FlushSuperBlock();

    m_Allocator.Initialize(this);

    m_RootEntry = new DirectoryEntry(nullptr, "/");
    auto* root  = new Ext2FsINode("/", this, 0644 | S_IFDIR);
    ReadINodeEntry(&root->m_Meta, 2);

    root->m_Metadata.ID        = 2;
    root->m_Metadata.DeviceID  = m_Device->Stats().st_rdev;
    root->m_Metadata.LinkCount = root->m_Meta.HardLinkCount;
    root->m_Metadata.Size      = root->m_Meta.SizeLow
                          | (static_cast<u64>(root->m_Meta.SizeHigh) << 32);
    root->m_Metadata.BlockCount
        = root->m_Metadata.Size / root->m_Metadata.BlockSize;

    if (!root)
    {
        LogError("Ext2Fs: '{}' -> Failed to create root node",
                 sourceEntry->Path());
        delete m_SuperBlock;
        return nullptr;
    }

    m_RootEntry->Bind(root);
    m_Root = root;
    return m_RootEntry;
}

ErrorOr<INode*> Ext2Fs::CreateNode(INode* parent, ::Ref<DirectoryEntry> entry,
                                   mode_t mode, uid_t uid, gid_t gid)
{
    usize inodeIndex = m_Allocator.AllocateINode();
    if (!inodeIndex) return nullptr;

    auto inode = new Ext2FsINode(entry->Name(), this, mode);
    if (!inode) return nullptr;

    inode->Initialize(inodeIndex, mode, Ext2Mode2INodeType(mode));

    // TODO(v1tr10l7): Defer allocation of inode blocks
    AssignINodeBlocks(inode->m_Meta, inodeIndex, 0, 1);
    WriteINodeEntry(inode->m_Meta, inodeIndex);

    Ext2FsINodeMeta parentMeta{};
    ReadINodeEntry(&parentMeta, parent->Stats().st_ino);

    if (S_ISDIR(mode))
    {
        u8*  buffer          = new u8[m_BlockSize];

        auto dotEntry        = reinterpret_cast<Ext2FsDirectoryEntry*>(buffer);
        dotEntry->INodeIndex = inodeIndex;
        dotEntry->Size       = 12;
        dotEntry->NameSize   = 1;
        dotEntry->Type       = Ext2FsDirectoryEntryType::eSymlink;
        dotEntry->Name[0]    = '.';

        auto dotDotEntry
            = reinterpret_cast<Ext2FsDirectoryEntry*>(buffer + dotEntry->Size);
        dotDotEntry->INodeIndex = parent->Stats().st_ino;
        dotDotEntry->Size       = m_BlockSize - dotEntry->Size;
        dotDotEntry->NameSize   = 2;
        dotDotEntry->Type       = Ext2FsDirectoryEntryType::eSymlink;
        dotDotEntry->Name[0]    = '.';
        dotDotEntry->Name[1]    = '.';

        WriteINode(inode->m_Meta, buffer, inodeIndex, 0, m_BlockSize);

        usize bgdIndex
            = (inode->m_Metadata.ID - 1) / m_SuperBlock->INodesPerGroup;
        Ext2FsBlockGroupDescriptor blockGroup{};
        ReadBlockGroupDescriptor(&blockGroup, bgdIndex);
        ++blockGroup.DirectoryCount;
        WriteBlockGroupDescriptor(blockGroup, bgdIndex);

        ++parentMeta.HardLinkCount;
        //++inode->m_Meta.HardLinkCount;
        WriteINodeEntry(parentMeta, parent->Stats().st_ino);
        WriteINodeEntry(inode->m_Meta, inodeIndex);
    }

    usize entrySize = Math::AlignUp(8 + entry->Name().Size(), 4);
    Ext2FsDirectoryEntry* dentry
        = Pointer(new u8[entrySize]).As<Ext2FsDirectoryEntry>();
    Memory::Fill(dentry, 0, entrySize);

    dentry->INodeIndex = inodeIndex;
    dentry->Size       = entrySize;
    dentry->NameSize   = entry->Name().Size();
    dentry->Type       = Ext2Mode2DirectoryEntryType(mode);
    StringView(reinterpret_cast<char*>(dentry->Name), dentry->NameSize)
        .Copy(const_cast<char*>(entry->Name().Raw()), entry->Name().Size());
    auto result
        = reinterpret_cast<Ext2FsINode*>(parent)->AddDirectoryEntry(*dentry);
    if (!result)
    {
        delete inode;
        // TODO(v1tr10l7): Free INode
        return nullptr;
    }

    return inode;
}

bool Ext2Fs::Populate(DirectoryEntry* dentry)
{
    Ext2FsINode*    e2node = reinterpret_cast<Ext2FsINode*>(dentry->INode());
    Ext2FsINodeMeta parentMeta;
    ReadINodeEntry(&parentMeta, e2node->m_Metadata.ID);

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
                         ToUnderlying(entry->Type));
                break;
        }

        Ext2FsINode* newNode          = new Ext2FsINode(nameBuffer, this, mode);
        newNode->m_Metadata.UID       = inodeMeta.UID;
        newNode->m_Metadata.GID       = inodeMeta.GID;
        newNode->m_Metadata.ID        = entry->INodeIndex;
        newNode->m_Metadata.Size      = inodeMeta.GetSize();
        newNode->m_Metadata.LinkCount = inodeMeta.HardLinkCount;
        newNode->m_Metadata.BlockCount = newNode->m_Metadata.Size / m_BlockSize;

        newNode->m_Metadata.AccessTime.tv_sec        = inodeMeta.AccessTime;
        newNode->m_Metadata.AccessTime.tv_nsec       = 0;
        newNode->m_Metadata.ChangeTime.tv_sec        = inodeMeta.CreationTime;
        newNode->m_Metadata.ChangeTime.tv_nsec       = 0;
        newNode->m_Metadata.ModificationTime.tv_sec  = inodeMeta.ModifiedTime;
        newNode->m_Metadata.ModificationTime.tv_nsec = 0;

        newNode->m_Meta                              = inodeMeta;

        e2node->InsertChild(newNode, newNode->Name());

        // TODO(v1tr10l7): resolve link
        if (newNode->IsSymlink())
            ;

        delete[] nameBuffer;
        i += entry->Size;
    }

    delete[] buffer;
    return true;
}

void Ext2Fs::FreeINode(usize inode)
{
    --inode;

    usize blockGroupIndex = inode / m_SuperBlock->INodesPerGroup;
    Ext2FsBlockGroupDescriptor blockGroup;
    ReadBlockGroupDescriptor(&blockGroup, blockGroupIndex);

    Bitmap bitmap;
    bitmap.Allocate(m_BlockSize);
    m_Device->Read(bitmap.Raw(),
                   blockGroup.INodeUsageBitmapAddress * m_BlockSize,
                   m_BlockSize);

    bitmap.SetIndex(inode % m_SuperBlock->INodesPerGroup, false);
    m_Device->Write(bitmap.Raw(),
                    blockGroup.INodeUsageBitmapAddress * m_BlockSize,
                    m_BlockSize);

    ++blockGroup.FreeINodeCount;
    ++m_SuperBlock->FreeINodeCount;

    WriteBlockGroupDescriptor(blockGroup, blockGroupIndex);
    FlushSuperBlock();

    bitmap.Free();
}

isize Ext2Fs::SetINodeBlock(Ext2FsINodeMeta& meta, u32 inode, u32 iblock,
                            u32 dblock)
{
    u32 blockLevel = m_BlockSize / 4;
    if (iblock < 12)
    {
        meta.Blocks[iblock] = dblock;
        return dblock;
    }

    iblock -= 12;
    if (iblock >= blockLevel)
    {
        iblock -= blockLevel;
        u32   singleIndex    = iblock / blockLevel;
        off_t indirectOffset = iblock % blockLevel;
        u32   indirectBlock  = 0;

        if (singleIndex >= blockLevel)
        {
            iblock -= blockLevel * blockLevel;

            u32 doubleIndirect      = iblock / blockLevel;
            indirectOffset          = iblock % blockLevel;
            u32 singleIndirectIndex = 0;

            if (meta.Blocks[14] == 0)
            {
                meta.Blocks[14] = AllocateBlock(meta, inode);
                WriteINodeEntry(meta, inode);
            }

            m_Device->Read(&singleIndirectIndex,
                           meta.Blocks[14] * m_BlockSize + doubleIndirect * 4,
                           sizeof(u32));

            if (singleIndirectIndex == 0)
            {
                singleIndirectIndex = AllocateBlock(meta, inode);
                m_Device->Write(&singleIndirectIndex,
                                meta.Blocks[14] * m_BlockSize
                                    + doubleIndirect * 4,
                                sizeof(u32));
            }

            m_Device->Read(&indirectBlock,
                           doubleIndirect * m_BlockSize
                               + singleIndirectIndex * 4,
                           sizeof(u32));

            if (indirectBlock == 0)
            {
                u32 newBlock = AllocateBlock(meta, inode);
                m_Device->Write(&indirectBlock,
                                doubleIndirect * m_BlockSize
                                    + singleIndirectIndex * 4,
                                sizeof(u32));

                indirectBlock = newBlock;
            }

            m_Device->Write(&dblock,
                            indirectBlock * m_BlockSize * indirectOffset * 4,
                            sizeof(u32));
            return dblock;
        }

        if (meta.Blocks[13] == 0)
        {
            meta.Blocks[13] = AllocateBlock(meta, inode);

            WriteINodeEntry(meta, inode);
        }

        m_Device->Read(&indirectBlock,
                       meta.Blocks[13] * m_BlockSize + singleIndex * 4,
                       sizeof(u32));

        if (indirectBlock == 0)
        {
            indirectBlock = AllocateBlock(meta, inode);

            m_Device->Write(&indirectBlock,
                            meta.Blocks[13] * m_BlockSize + singleIndex * 4,
                            sizeof(u32));
        }

        m_Device->Write(&dblock,
                        indirectBlock * m_BlockSize + indirectOffset * 4,
                        sizeof(u32));

        return dblock;
    }

    if (meta.Blocks[12] == 0)
    {
        meta.Blocks[12] = AllocateBlock(meta, inode);

        WriteINodeEntry(meta, inode);
    }
    m_Device->Write(&dblock, meta.Blocks[12] * m_BlockSize + iblock * 4,
                    sizeof(u32));
    return dblock;
}
void Ext2Fs::AssignINodeBlocks(Ext2FsINodeMeta& meta, u32 inode, usize start,
                               usize blocks)
{
    for (usize i = 0; i < blocks; i++)
    {
        if (GetINodeBlock(meta, start + i)) continue;

        usize dblock = AllocateBlock(meta, inode);
        SetINodeBlock(meta, inode, start + i, dblock);
    }

    WriteINodeEntry(meta, inode);
}
isize Ext2Fs::GrowINode(Ext2FsINodeMeta& meta, u32 inode, usize start,
                        usize count)
{
    usize blockOffset = start / m_BlockSize;
    usize blockCount  = (count + m_BlockSize - 1) / m_BlockSize;
    // usize blockOffset = (start & ~(m_BlockSize - 1)) >> (10 + m_BlockSize);
    // usize blockCount = ((start & (m_BlockSize - 1)) + count + (m_BlockSize -
    // 1))
    //                 >> (10 + m_BlockSize);

    AssignINodeBlocks(meta, inode, blockOffset, blockCount);
    return 0;
}

usize Ext2Fs::AllocateBlock(Ext2FsINodeMeta& meta, u32 inode)
{
    usize allocatedBlock = m_Allocator.AllocateBlock(meta, inode);
    if (!allocatedBlock) return 0;

    meta.SectorCount += m_BlockSize / m_Device->Stats().st_blksize;
    WriteINodeEntry(meta, inode);

    --m_SuperBlock->FreeBlockCount;
    FlushSuperBlock();

    return allocatedBlock;
}
void Ext2Fs::FreeBlock(usize block)
{
    usize blockGroupIndex = block / m_SuperBlock->BlocksPerGroup;
    Ext2FsBlockGroupDescriptor blockGroup;
    ReadBlockGroupDescriptor(&blockGroup, blockGroupIndex);

    Bitmap bitmap;
    bitmap.Allocate(m_BlockSize);
    m_Device->Read(bitmap.Raw(),
                   blockGroup.BlockUsageBitmapAddress * m_BlockSize,
                   m_BlockSize);
    bitmap.SetIndex(block % m_SuperBlock->BlocksPerGroup, false);
    m_Device->Write(bitmap.Raw(),
                    blockGroup.BlockUsageBitmapAddress * m_BlockSize,
                    m_BlockSize);

    ++blockGroup.FreeBlockCount;
    ++m_SuperBlock->FreeBlockCount;

    WriteBlockGroupDescriptor(blockGroup, blockGroupIndex);
    FlushSuperBlock();

    bitmap.Free();
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
isize Ext2Fs::WriteINode(Ext2FsINodeMeta& meta, u8* in, u32 inode, off_t offset,
                         usize count)
{
    GrowINode(meta, inode, offset, count);

    if (offset + count > meta.GetSize())
    {
        meta.SetSize(offset + count);
        WriteINodeEntry(meta, inode);
    }

    for (usize head = 0; head < count;)
    {
        usize iblock    = (offset + head) / m_BlockSize;

        usize size      = count - head;
        off_t subOffset = (offset + head) % m_BlockSize;

        if (size > (m_BlockSize - subOffset)) size = m_BlockSize - subOffset;

        usize dblock = GetINodeBlock(meta, iblock);

        m_Device->Write(in + head, dblock * m_BlockSize + subOffset, size);
        head += size;
    }

    return count;
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

ScopedLock&& Ext2Fs::LockSuperBlock()
{
    ScopedLock guard(m_Lock);

    return Move(guard);
}
void Ext2Fs::ReadSuperBlock()
{
    ScopedLock guard(m_Lock);

    m_Device->Read(m_SuperBlock, 1024, sizeof(Ext2FsSuperBlock));
}
void Ext2Fs::FlushSuperBlock()
{
    ScopedLock guard(m_Lock);

    m_Device->Write(m_SuperBlock, 1024, sizeof(Ext2FsSuperBlock));
}
void Ext2Fs::ReadBlockGroupDescriptor(Ext2FsBlockGroupDescriptor* out,
                                      usize                       index)
{
    off_t offset = m_BlockSize >= 2048 ? m_BlockSize : m_BlockSize * 2;

    m_Device->Read(out, offset + sizeof(Ext2FsBlockGroupDescriptor) * index,
                   sizeof(Ext2FsBlockGroupDescriptor));
}
void Ext2Fs::WriteBlockGroupDescriptor(Ext2FsBlockGroupDescriptor& in,
                                       usize                       index)
{
    off_t offset = m_BlockSize >= 2048 ? m_BlockSize : m_BlockSize * 2;

    m_Device->Write(&in, offset + sizeof(Ext2FsBlockGroupDescriptor) * index,
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

        u32 singleIndex    = blockIndex / blockLevel;
        i64 indirectOffset = blockIndex % blockLevel;
        u32 indirectBlock  = 0;

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
