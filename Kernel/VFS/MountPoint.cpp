/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/Filesystem.hpp>
#include <VFS/MountPoint.hpp>

MountPoint::List MountPoint::s_MountPoints = {};

MountPoint::MountPoint(::Ref<DirectoryEntry>   hostEntry,
                       ::Ref<class Filesystem> mountedFs)
    : m_Root(hostEntry)
    , m_Filesystem(mountedFs)
{
}

::Ref<DirectoryEntry> MountPoint::HostEntry() const { return m_Root; }
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
        if (iterator(current)) break;
}

::Ref<MountPoint> MountPoint::NextMountPoint() const { return Hook.Next; }
