/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>
#include <Common.hpp>

#include <Prism/Containers/Vector.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/Path.hpp>

class INode;
class DirectoryEntry;
class MountPoint;
class FileDescriptor;

namespace VFS
{
    Vector<std::pair<bool, StringView>>& Filesystems();

    Ref<DirectoryEntry>                      GetRootDirectoryEntry();
    void                                 RecursiveDelete(INode* node);

    struct PathResolution
    {
        DirectoryEntry* Parent   = nullptr;
        DirectoryEntry* Entry    = nullptr;
        Path            BaseName = ""_s;
    };

    ErrorOr<FileDescriptor*> Open(DirectoryEntry* parent, PathView path,
                                  i32 flags, mode_t mode);

    ErrorOr<PathResolution>  ResolvePath(DirectoryEntry* parent, PathView path,
                                         bool followLinks = true);

    ErrorOr<MountPoint*>     MountRoot(StringView filesystemName);
    ErrorOr<MountPoint*>     Mount(DirectoryEntry* parent, PathView source,
                                   PathView target, StringView fsName,
                                   i32 flags = 0, const void* data = nullptr);
    bool Unmount(DirectoryEntry* parent, PathView path, i32 flags = 0);

    Ref<DirectoryEntry> CreateNode(Ref<DirectoryEntry> parent, PathView path,
                                   mode_t mode);
    ErrorOr<Ref<DirectoryEntry>> MkDir(Ref<DirectoryEntry> directory,
                                       Ref<DirectoryEntry> entry, mode_t mode);
    ErrorOr<Ref<DirectoryEntry>> MkDir(DirectoryEntry* directory, PathView path,
                                       mode_t mode);

    ErrorOr<Ref<DirectoryEntry>> MkNod(Ref<DirectoryEntry> parent,
                                       PathView name, mode_t mode, dev_t dev);
    ErrorOr<Ref<DirectoryEntry>> MkNod(PathView path, mode_t mode, dev_t dev);
    Ref<DirectoryEntry>          Symlink(DirectoryEntry* parent, PathView path,
                                         StringView target);
    Ref<DirectoryEntry> Link(DirectoryEntry* oldParent, PathView oldPath,
                             DirectoryEntry* newParent, PathView newPath,
                             i32 flags = 0);

    bool Unlink(DirectoryEntry* parent, PathView path, i32 flags = 0);
}; // namespace VFS
