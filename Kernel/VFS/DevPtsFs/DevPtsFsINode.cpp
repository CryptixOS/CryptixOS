/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/sys/stat.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/Optional.hpp>
#include <Prism/Utility/Math.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/linux/capability.h>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.DevPtsFs.DevPtsFsINode;

import :Drivers.Core.DeviceManager;
import :Scheduler.Process;
import :VFS.DevPtsFs.DevPtsFs;
import :VFS.DevPtsFs.DevPtsFsINode;
import :VFS.Filesystem;
import :Time.Time;

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
