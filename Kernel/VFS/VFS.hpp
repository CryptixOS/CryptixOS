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
#include <Prism/String/String.hpp>
#include <Prism/Utility/Path.hpp>

class INode;
class DirectoryEntry;
class MountPoint;
class FileDescriptor;

namespace VFS
{
    Vector<std::pair<bool, StringView>>& Filesystems();

    DirectoryEntry*                      GetRootDirectoryEntry();
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

    DirectoryEntry*          CreateNode(DirectoryEntry* parent, PathView path,
                                        mode_t mode);
    ErrorOr<DirectoryEntry*> MkDir(DirectoryEntry* directory,
                                   DirectoryEntry* entry, mode_t mode);
    ErrorOr<DirectoryEntry*> MkDir(DirectoryEntry* directory, PathView path,
                                   mode_t mode);

    ErrorOr<DirectoryEntry*> MkNod(DirectoryEntry* parent, PathView name,
                                   mode_t mode, dev_t dev);
    ErrorOr<DirectoryEntry*> MkNod(PathView path, mode_t mode, dev_t dev);
    DirectoryEntry*          Symlink(DirectoryEntry* parent, PathView path,
                                     StringView target);
    DirectoryEntry*          Link(DirectoryEntry* oldParent, PathView oldPath,
                                  DirectoryEntry* newParent, PathView newPath,
                                  i32 flags = 0);

    bool Unlink(DirectoryEntry* parent, PathView path, i32 flags = 0);
}; // namespace VFS
