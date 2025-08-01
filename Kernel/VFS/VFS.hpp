/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>
#include <Common.hpp>

#include <Drivers/Core/FilesystemDriver.hpp>

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
    void                           Initialize();
    bool                           IsInitialized();

    ErrorOr<Ref<FilesystemDriver>> FindFilesystem(StringView name);

    ErrorOr<void> RegisterFilesystem(Ref<FilesystemDriver> driver);
    ErrorOr<void> UnregisterFilesystem(Ref<FilesystemDriver> driver);

    Vector<std::pair<bool, StringView>>& Filesystems();

    Ref<DirectoryEntry>                  RootDirectoryEntry();
    void                                 RecursiveDelete(INode* node);

    struct PathResolution
    {
        Ref<DirectoryEntry> Parent   = nullptr;
        Ref<DirectoryEntry> Entry    = nullptr;
        Path                BaseName = ""_s;
    };

    ErrorOr<Ref<DirectoryEntry>> OpenDirectoryEntry(Ref<DirectoryEntry> parent,
                                                    PathView path, isize flags,
                                                    mode_t mode);
    ErrorOr<Ref<FileDescriptor>> Open(Ref<DirectoryEntry> parent, PathView path,
                                      isize flags, mode_t mode);

    ErrorOr<PathResolution>      ResolvePath(Ref<DirectoryEntry> parent,
                                             PathView path, bool followLinks = true);
    ErrorOr<Ref<DirectoryEntry>> ResolveParent(Ref<DirectoryEntry> parent,
                                               PathView            path);

    ErrorOr<Ref<MountPoint>>     MountRoot(StringView filesystemName);
    ErrorOr<Ref<MountPoint>> Mount(Ref<DirectoryEntry> parent, PathView source,
                                   PathView target, StringView fsName,
                                   i32 flags = 0, const void* data = nullptr);
    bool Unmount(Ref<DirectoryEntry> parent, PathView path, i32 flags = 0);

    ErrorOr<void>                Sync();

    ErrorOr<Ref<DirectoryEntry>> CreateNode(Ref<DirectoryEntry> parent,
                                            StringView name, mode_t mode,
                                            dev_t dev, PathView target = ""_pv);
    ErrorOr<Ref<DirectoryEntry>> CreateNode(PathView path, mode_t mode,
                                            dev_t dev);

    ErrorOr<Ref<DirectoryEntry>> CreateFile(Ref<DirectoryEntry> directory,
                                            StringView name, mode_t mode);
    ErrorOr<Ref<DirectoryEntry>> CreateFile(PathView path, mode_t mode);

    ErrorOr<Ref<DirectoryEntry>> CreateDirectory(Ref<DirectoryEntry> directory,
                                                 StringView name, mode_t mode);
    ErrorOr<Ref<DirectoryEntry>> CreateDirectory(PathView path, mode_t mode);

    ErrorOr<Ref<DirectoryEntry>> Symlink(Ref<DirectoryEntry> directory,
                                         StringView name, PathView targetPath);
    ErrorOr<Ref<DirectoryEntry>> Symlink(PathView path, PathView targetPath);

    ErrorOr<Ref<DirectoryEntry>> Link(Ref<DirectoryEntry> oldParent,
                                      StringView          oldName,
                                      Ref<DirectoryEntry> newParent,
                                      StringView newName, i32 flags = 0);
    ErrorOr<Ref<DirectoryEntry>> Link(PathView oldPath, PathView newPath,
                                      i32 flags = 0);

    bool Unlink(Ref<DirectoryEntry> parent, PathView path, i32 flags = 0);
}; // namespace VFS
