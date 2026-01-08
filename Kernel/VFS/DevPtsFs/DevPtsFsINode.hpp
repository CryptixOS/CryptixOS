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

class DevPtsFsINode final : public SynthFsINode, NonCopyable<DevPtsFsINode>
{
  public:
    DevPtsFsINode(StringView name, class Filesystem* fs, INodeID id,
                  INodeMode mode, Device* device = nullptr);
    virtual ~DevPtsFsINode() {}

    virtual ErrorOr<::Ref<DirectoryEntry>>
                  CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode,
                             ::DeviceID dev) override;

    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> IoCtl(usize request, usize arg) override;

  private:
    Device* m_Device = nullptr;
    friend class DevPtsFs;
};
