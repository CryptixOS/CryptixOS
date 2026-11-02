/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/termios.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
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
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.Drivers.TTY.PTYMultiplexer;

import :API.DeviceIDs;
import :Drivers.Core.DeviceManager;
import :Drivers.TTY.MasterPTY;
import :Drivers.TTY.PTYMultiplexer;
import :VFS.VFS;

PTYMultiplexer* PTYMultiplexer::s_PTMX = nullptr;

PTYMultiplexer::PTYMultiplexer(DeviceMinor minor)
    : CharacterDevice("ptmx", MakeDevice(API::DeviceMajor::TTYAUX, minor))
{
}
PTYMultiplexer::~PTYMultiplexer() {}

static INodeID NewPTSIndex()
{
    static Atomic<usize> s_NextID = 0;
    return ++s_NextID;
}
ErrorOr<File*> PTYMultiplexer::Open(File* file)
{
    LogTrace("PTYMultiplexer: Open called");
    auto   newIndex = NewPTSIndex();
    String ptyName  = fmt::format("pts{}", newIndex).data();
    auto   master   = new MasterPTY(ptyName, newIndex);

    DeviceManager::RegisterCharDevice(master);
    auto ptyPath = fmt::format("/dev/pts/{}", ptyName).data();
    VFS::CreateNode(ptyPath, S_IFCHR | 0666, master->ID());

    // TODO(v1tr10l7): Open new pty master/slave pair
    return master;
}
