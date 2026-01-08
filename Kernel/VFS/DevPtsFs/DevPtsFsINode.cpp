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
    : SynthFsINode(name, fs, id, mode)
    , m_Device(device)
{
}

ErrorOr<::Ref<DirectoryEntry>>
DevPtsFsINode::CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode,
                          dev_t dev)
{
    auto dentry = TryOrRet(SynthFsINode::CreateNode(entry, mode, dev));
    auto inode  = entry->INode().As<DevPtsFsINode>();

    if (inode->IsCharDevice())
    {
        auto device
            = reinterpret_cast<Device*>(DeviceManager::LookupCharDevice(dev));

        inode->m_Device = device;
    }
    else if (inode->IsBlockDevice())
    {
        auto device
            = reinterpret_cast<Device*>(DeviceManager::LookupBlockDevice(dev));

        inode->m_Device = device;
    }

    return entry;
}

isize DevPtsFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (m_Device)
    {
        auto result = m_Device->Read(buffer, offset, bytes);
        return result ? result.Value() : -1;
    }

    return SynthFsINode::Read(buffer, offset, bytes);
}
isize DevPtsFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    if (!buffer) return_err(-1, EFAULT);
    if (m_Device)
    {
        auto result = m_Device->Write(buffer, offset, bytes);
        return result ? result.Value() : -1;
    }

    return SynthFsINode::Write(buffer, offset, bytes);
}

ErrorOr<isize> DevPtsFsINode::IoCtl(usize request, usize arg)
{
    if (!m_Device) return Error(ENOTTY);

    return m_Device->IoCtl(request, arg);
}
