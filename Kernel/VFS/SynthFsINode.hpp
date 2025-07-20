/*
 * Created by v1tr10l7 on 28.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Memory/Buffer.hpp>

#include <VFS/INode.hpp>

class SynthFsINode : public INode
{
  public:
    SynthFsINode(INode* parent, StringView name,
                 class Filesystem* filesystem, mode_t mode);
    virtual ~SynthFsINode() = default;

    virtual void  InsertChild(INode* node, StringView name) override {}

    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

  private:
    Buffer                 m_Buffer;
    constexpr static usize DEFAULT_SIZE = 4096;
};
