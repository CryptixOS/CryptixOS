/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/MountPoint.hpp>

MountPoint::List MountPoint::s_MountPoints = {};

MountPoint::MountPoint(DirectoryEntry* hostEntry, class Filesystem* mountedFs)
    : m_Root(hostEntry)
    , m_Filesystem(mountedFs)
{
}

void MountPoint::Attach(MountPoint* mountPoint)
{
    s_MountPoints.PushBack(mountPoint);
}
MountPoint* MountPoint::Head() { return s_MountPoints.Head(); }

MountPoint* MountPoint::NextMountPoint() const { return Hook.Next; }
