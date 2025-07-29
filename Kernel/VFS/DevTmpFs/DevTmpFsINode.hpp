/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
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

using DeviceID = dev_t;

class DevTmpFsINode final : public SynthFsINode, NonCopyable<DevTmpFsINode>
{
  public:
    DevTmpFsINode(StringView name, class Filesystem* fs, INodeID id,
                  INodeMode mode, Device* device = nullptr);
    virtual ~DevTmpFsINode() {}

    virtual ErrorOr<Ref<DirectoryEntry>>
    CreateNode(Ref<DirectoryEntry> entry, mode_t mode, dev_t dev) override;

    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> IoCtl(usize request, usize arg) override;

  private:
    Device* m_Device = nullptr;
    friend class DevTmpFs;
};
