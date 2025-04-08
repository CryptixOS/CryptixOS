/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/Filesystem.hpp>

struct EchFsIdentityTable
{
    u8  JumpInstruction[4];
    u8  Signature[8];
    u64 TotalBlockCount;
    u64 MainDirectoryLength;
    u64 BytesPerBlock;
    u64 Reserved;
    u64 UUID[2];
} __attribute__((packed));

constexpr usize ECHFS_END_OF_DIRECTORY  = 0x0000'0000'0000'0000;
constexpr usize ECHFS_DELETED_DIRECTORY = 0xffff'ffff'ffff'fffe;
constexpr usize ECHFS_ROOT_DIRECTORY    = 0xffff'ffff'ffff'ffff;

class EchFs     final : public Filesystem
{
  public:
    EchFs(u32 flags)
        : Filesystem("EchFs", flags)
    {
    }
    virtual ~EchFs() = default;

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
    virtual bool Populate(INode* node) override;

  private:
    INode* m_Device              = nullptr;
    usize  m_BlockSize           = 0;
    usize  m_MainDirectoryOffset = 0;
    usize  m_MainDirectoryLength = 0;
};
