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

class Ext2Fs : public Filesystem
{
  public:
    Ext2Fs(u32 flags)
        : Filesystem("Ext2Fs", flags)
    {
    }
    virtual ~Ext2Fs() = default;

    virtual ErrorOr<INode*> Mount(INode* parent, INode* source, INode* target,
                                  DirectoryEntry* entry, StringView name,
                                  const void* data = nullptr) override;
    virtual ErrorOr<INode*> CreateNode(INode* parent, DirectoryEntry* entry,
                                       mode_t mode, uid_t uid = 0,
                                       gid_t gid = 0) override;
    virtual ErrorOr<INode*> Symlink(INode* parent, DirectoryEntry* entry,
                                    StringView target) override
    {
        return nullptr;
    }

    virtual INode* Link(INode* parent, StringView name, INode* oldNode) override
    {
        return nullptr;
    }
    virtual bool             Populate(INode* node) override;

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
    INode*            m_Device = nullptr;
    // u64               m_DeviceID;

    Ext2FsSuperBlock* m_SuperBlock;
    usize             m_BlockSize;
    usize             m_FragmentSize;
    usize             m_BlockGroupDescriptionCount;
    friend class Ext2FsAllocator;
    Ext2FsAllocator m_Allocator;

    ScopedLock&&    LockSuperBlock();
    void            ReadSuperBlock();
    void            FlushSuperBlock();

    void ReadBlockGroupDescriptor(Ext2FsBlockGroupDescriptor* out, usize index);
    void WriteBlockGroupDescriptor(Ext2FsBlockGroupDescriptor& in, usize index);
    u32  GetINodeBlock(Ext2FsINodeMeta& meta, u32 blockIndex);
};
