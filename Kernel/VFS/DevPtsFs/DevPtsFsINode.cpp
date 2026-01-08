/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/DeviceManager.hpp>
#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <VFS/DevPtsFs/DevPtsFs.hpp>
#include <VFS/DevPtsFs/DevPtsFsINode.hpp>
#include <VFS/Filesystem.hpp>

#include <Time/Time.hpp>

DevPtsFsINode::DevPtsFsINode(StringView name, class Filesystem* fs, INodeID id,
                             INodeMode mode, Device* device)
    : INode(name, fs)
    , m_Device(device)
{
    m_Metadata.ID           = id;
    m_Metadata.Mode         = mode;

    m_Metadata.Size         = 0;
    m_Metadata.LinkCount    = 1;

    m_Metadata.BlockSize    = PMM::PAGE_SIZE;
    m_Metadata.BlockCount   = 0;

    m_Metadata.RootDeviceID = fs->BackingDeviceID();
    m_Metadata.DeviceID     = device ? device->ID() : 0;
}

ErrorOr<::Ref<DirectoryEntry>>
DevPtsFsINode::CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode,
                          dev_t dev)
{
    return Error(ENOENT);
}

void DevPtsFsINode::InsertChild(::Ref<INode> node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}
isize DevPtsFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (!m_Device) return_err(-1, ENODEV);

    auto result = m_Device->Read(buffer, offset, bytes);
    return result ? result.Value() : -1;
}
isize DevPtsFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (!m_Device) return_err(-1, ENODEV);

    auto result = m_Device->Write(buffer, offset, bytes);
    return result ? result.Value() : -1;
}

ErrorOr<isize> DevPtsFsINode::IoCtl(usize request, usize arg)
{
    if (!m_Device) return Error(ENOTTY);

    return m_Device->IoCtl(request, arg);
}
