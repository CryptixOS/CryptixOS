/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>

class DirectoryEntry;
class Filesystem;
class MountPoint : public RefCounted
{
  public:
    using MountPointIterator = Delegate<bool(::Ref<MountPoint> mountPoint)>;

    explicit MountPoint(::Ref<DirectoryEntry> hostEntry  = nullptr,
               ::Ref<Filesystem>     filesystem = nullptr);

    ::Ref<DirectoryEntry>    HostEntry() const;
    ::Ref<Filesystem>        Filesystem() const;

    static void              Attach(::Ref<MountPoint> mountPoint);
    static ::Ref<MountPoint> Lookup(::Ref<DirectoryEntry> entry);

    static ::Ref<MountPoint> Head();
    static ::Ref<MountPoint> Tail();
    static void              Iterate(MountPointIterator iterator);

    ::Ref<MountPoint>        NextMountPoint() const;

  private:
    ::Ref<DirectoryEntry>   m_Root       = nullptr;
    ::Ref<class Filesystem> m_Filesystem = nullptr;

    friend class IntrusiveRefList<MountPoint>;
    friend struct IntrusiveRefListHook<MountPoint>;

    IntrusiveRefListHook<MountPoint> Hook;

    using List = IntrusiveRefList<MountPoint>;
    static List s_MountPoints;
};
