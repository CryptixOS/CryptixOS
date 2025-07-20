/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Device.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Memory/Buffer.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/INode.hpp>

class DevTmpFsINode : public INode, NonCopyable<DevTmpFsINode>
{
  public:
    DevTmpFsINode(StringView name, class Filesystem* fs, mode_t mode,
                  Device* device = nullptr);
    virtual ~DevTmpFsINode() {}

    virtual const stat Stats() override
    {
        if (m_Device) return m_Device->Stats();

        stat stats{};
        stats.st_dev     = m_Metadata.DeviceID;
        stats.st_ino     = m_Metadata.ID;
        stats.st_nlink   = m_Metadata.LinkCount;
        stats.st_mode    = m_Metadata.Mode;
        stats.st_uid     = m_Metadata.UID;
        stats.st_gid     = m_Metadata.GID;
        stats.st_rdev    = m_Metadata.RootDeviceID;
        stats.st_size    = m_Metadata.Size;
        stats.st_blksize = m_Metadata.BlockSize;
        stats.st_blocks  = m_Metadata.BlockCount;
        stats.st_atim    = m_Metadata.AccessTime;
        stats.st_mtim    = m_Metadata.ModificationTime;
        stats.st_ctim    = m_Metadata.ChangeTime;
        return stats;
    }

    virtual ErrorOr<void>
    TraverseDirectories(Ref<class DirectoryEntry> parent,
                        DirectoryIterator         iterator) override;
    virtual ErrorOr<Ref<DirectoryEntry>>
    Lookup(Ref<DirectoryEntry> dentry) override;

    const UnorderedMap<StringView, INode*>& Children() const
    {
        return m_Children;
    }
    virtual void InsertChild(INode* node, StringView name) override
    {
        ScopedLock guard(m_Lock);
        m_Children[name] = node;
    }

    virtual ErrorOr<Ref<DirectoryEntry>>
    CreateNode(Ref<DirectoryEntry> entry, mode_t mode, dev_t dev = 0) override;
    virtual ErrorOr<Ref<DirectoryEntry>> CreateFile(Ref<DirectoryEntry> entry,
                                                    mode_t mode) override;
    virtual ErrorOr<Ref<DirectoryEntry>>
    CreateDirectory(Ref<DirectoryEntry> entry, mode_t mode) override;
    virtual ErrorOr<Ref<DirectoryEntry>> Symlink(Ref<DirectoryEntry> entry,
                                                 PathView targetPath) override;

    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> IoCtl(usize request, usize arg) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

  private:
    Device*                          m_Device = nullptr;

    Buffer                           m_Buffer;
    UnorderedMap<StringView, INode*> m_Children;
    friend class DevTmpFs;
};
