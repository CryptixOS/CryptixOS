/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Delegate.hpp>

class DirectoryEntry;
class Filesystem;
class MountPoint
{
  public:
    using MountPointIterator = Delegate<bool(MountPoint* mountPoint)>;

    MountPoint(DirectoryEntry* hostEntry  = nullptr,
               Filesystem*     filesystem = nullptr);

    inline constexpr DirectoryEntry* HostEntry() const { return m_Root; }
    inline constexpr Filesystem*     Filesystem() const { return m_Filesystem; }

    static void                      Attach(MountPoint* mountPoint);
    static MountPoint*               Lookup(DirectoryEntry* entry);

    static MountPoint*               Head();
    static MountPoint*               Tail();
    static void                      Iterate(MountPointIterator iterator);

    MountPoint*                      NextMountPoint() const;

  private:
    DirectoryEntry*   m_Root       = nullptr;
    class Filesystem* m_Filesystem = nullptr;

    friend class IntrusiveList<MountPoint>;
    friend struct IntrusiveListHook<MountPoint>;

    IntrusiveListHook<MountPoint> Hook;

    using List = IntrusiveList<MountPoint>;
    static List s_MountPoints;
};
