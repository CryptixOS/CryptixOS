/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

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

    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name, void* data = nullptr) override;
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
    virtual bool             Populate(INode* node) override;

    inline Ext2FsSuperBlock* GetSuperBlock() const { return m_SuperBlock; }
    inline usize             GetBlockSize() const { return m_BlockSize; }

    usize                    AllocateINode();
    void                     FreeINode(usize inode);

    usize                    AllocateBlock(Ext2FsINodeMeta& meta, u32 inode);
    void                     FreeBlock(usize block);

    void                     ReadINodeEntry(Ext2FsINodeMeta* out, u32 index);
    void                     WriteINodeEntry(Ext2FsINodeMeta& in, u32 index);
    isize ReadINode(Ext2FsINodeMeta& meta, u8* out, off_t offset, usize bytes);

  private:
    INode*            m_Device = nullptr;
    // u64               m_DeviceID;

    Ext2FsSuperBlock* m_SuperBlock;
    usize             m_BlockSize;
    usize             m_FragmentSize;
    usize             m_BlockGroupDescriptionCount;

    void ReadBlockGroupDescriptor(Ext2FsBlockGroupDescriptor* out, usize index);
    void WriteBlockGroupDescriptor(Ext2FsBlockGroupDescriptor& in, usize index);
    u32  GetINodeBlock(Ext2FsINodeMeta& meta, u32 blockIndex);
};
