/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/Ext2Fs/Ext2FsAllocator.hpp>
#include <VFS/Ext2Fs/Ext2FsStructures.hpp>

#include <VFS/Filesystem.hpp>
#include <VFS/INode.hpp>

class Ext2FsINode;
class Ext2Fs : public Filesystem
{
  public:
    explicit Ext2Fs(u32 flags)
        : Filesystem("Ext2Fs", flags)
    {
    }
    virtual ~Ext2Fs() = default;

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;
    ErrorOr<INode*> CreateNode(INode* parent, ::Ref<DirectoryEntry> entry,
                               mode_t mode, uid_t uid = 0, gid_t gid = 0);
    virtual bool    Populate(DirectoryEntry* dentry) override;

    inline Ext2FsSuperBlock* GetSuperBlock() const { return m_SuperBlock; }
    inline usize             GetBlockSize() const { return m_BlockSize; }

    void                     FreeINode(usize inode);

    isize SetINodeBlock(Ext2FsINodeMeta& meta, u32 inode, u32 iblock,
                        u32 dblock);
    void  AssignINodeBlocks(Ext2FsINodeMeta& meta, u32 inode, usize start,
                            usize blocks);
    isize GrowINode(Ext2FsINodeMeta& meta, u32 inode, usize start, usize count);

    usize AllocateBlock(Ext2FsINodeMeta& meta, u32 inode);
    void  FreeBlock(usize block);

    void  ReadINodeEntry(Ext2FsINodeMeta* out, u32 index);
    void  WriteINodeEntry(Ext2FsINodeMeta& in, u32 index);
    isize ReadINode(Ext2FsINodeMeta& meta, u8* out, off_t offset, usize bytes);
    isize WriteINode(Ext2FsINodeMeta& meta, u8* in, u32 inode, off_t offset,
                     usize count);

  private:
    INode*            m_Device                     = nullptr;
    // u64               m_DeviceID;

    Ext2FsSuperBlock* m_SuperBlock                 = nullptr;
    usize             m_BlockSize                  = 0;
    usize             m_FragmentSize               = 0;
    usize             m_BlockGroupDescriptionCount = 0;
    friend class Ext2FsAllocator;
    Ext2FsAllocator m_Allocator;

    ScopedLock&&    LockSuperBlock();
    void            ReadSuperBlock();
    void            FlushSuperBlock();

    void ReadBlockGroupDescriptor(Ext2FsBlockGroupDescriptor* out, usize index);
    void WriteBlockGroupDescriptor(Ext2FsBlockGroupDescriptor& in, usize index);
    u32  GetINodeBlock(Ext2FsINodeMeta& meta, u32 blockIndex);
};
