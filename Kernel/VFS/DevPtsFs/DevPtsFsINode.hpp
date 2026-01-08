/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/Device.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Memory/Buffer.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/SynthFsINode.hpp>

using DeviceID = DeviceID;

class DevPtsFsINode final : public INode, public NonCopyable<DevPtsFsINode>
{
  public:
    DevPtsFsINode(StringView name, class Filesystem* fs, INodeID id,
                  INodeMode mode, Device* device = nullptr);
    virtual ~DevPtsFsINode() {}

    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode,
               ::DeviceID dev) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateFile(::Ref<DirectoryEntry> entry, mode_t mode) override
    {
        return Error(ENOENT);
    }
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateDirectory(::Ref<DirectoryEntry> entry, mode_t mode) override
    {
        return Error(ENOENT);
    }

    virtual ErrorOr<::Ref<DirectoryEntry>> Symlink(::Ref<DirectoryEntry> entry,
                                                   PathView targetPath) override
    {
        return Error(ENOENT);
    }

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Link(::Ref<DirectoryEntry> oldEntry, ::Ref<DirectoryEntry> entry) override
    {
        return Error(ENOENT);
    }

    virtual void  InsertChild(::Ref<INode> node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> IoCtl(usize request, usize arg) override;
    virtual ErrorOr<Path>  ReadLink() override { return Error(ENOENT); }

    virtual ErrorOr<isize> Truncate(usize size) override
    {
        return Error(ENOENT);
    }
    virtual ErrorOr<void> Rename(::Ref<INode> newParent,
                                 StringView   newName) override
    {
        return Error(ENOENT);
    }

    virtual ErrorOr<void> Unlink(::Ref<DirectoryEntry> entry) override
    {
        return Error(ENOENT);
    }

    virtual ErrorOr<void> RmDir(::Ref<DirectoryEntry> entry) override
    {
        return Error(ENOENT);
    }

  private:
    Device*                                m_Device = nullptr;
    UnorderedMap<StringView, ::Ref<INode>> m_Children;

    friend class DevPtsFs;
};
