/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/sys/statfs.h>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/String/String.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

module Cryptix:Impl.VFS.MountPoint;

import :VFS.Filesystem;
import :VFS.MountPoint;

MountPoint::List MountPoint::s_MountPoints = {};

MountPoint::MountPoint(::Ref<DirectoryEntry>   hostEntry,
                       ::Ref<class Filesystem> mountedFs)
    : m_Host(hostEntry)
    , m_Guest(mountedFs->RootDirectoryEntry())
    , m_Filesystem(mountedFs)
{
}

::Ref<DirectoryEntry> MountPoint::HostEntry() const { return m_Host; }
::Ref<DirectoryEntry> MountPoint::GuestEntry() const { return m_Guest; }
::Ref<Filesystem>     MountPoint::Filesystem() const { return m_Filesystem; }

void                  MountPoint::Attach(::Ref<MountPoint> mountPoint)
{
    s_MountPoints.PushBack(mountPoint);
}
::Ref<MountPoint> MountPoint::Lookup(::Ref<DirectoryEntry> entry)
{
    auto current = Head();
    while (current && current->HostEntry() != entry)
        current = current->NextMountPoint();

    return current;
}

::Ref<MountPoint> MountPoint::Head() { return s_MountPoints.Head(); }
::Ref<MountPoint> MountPoint::Tail() { return s_MountPoints.Tail(); }
void              MountPoint::Iterate(Iterator iterator)
{
    auto current = Head();
    for (; current; current = current->NextMountPoint())
        if (iterator(current) == IterationResult::eBreak) break;
}

::Ref<MountPoint>     MountPoint::NextMountPoint() const { return Hook.Next; }
::Ref<DirectoryEntry> MountPoint::ExchangeGuest(::Ref<DirectoryEntry> dentry)
{
    m_Filesystem = dentry->INode()->Filesystem();
    return Exchange(m_Guest, dentry);
}
