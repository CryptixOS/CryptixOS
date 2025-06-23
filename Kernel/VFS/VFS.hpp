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
#include <Prism/Path.hpp>
#include <Prism/String/String.hpp>

#include <unordered_map>

class INode;
class DirectoryEntry;
class FileDescriptor;
namespace VFS
{
    Vector<std::pair<bool, StringView>>& GetFilesystems();

    DirectoryEntry*                      GetRootDirectoryEntry();
    void                                 RecursiveDelete(INode* node);

    struct PathResolution
    {
        INode* Parent   = nullptr;
        INode* Node     = nullptr;

        Path   BaseName = ""_s;
    };
    struct PathRes
    {
        DirectoryEntry* Parent   = nullptr;
        DirectoryEntry* Node     = nullptr;
        Path            BaseName = ""_s;
    };

    ErrorOr<FileDescriptor*> Open(DirectoryEntry* parent, PathView path,
                                  i32 flags, mode_t mode);

    ErrorOr<INode*>          ResolvePath(PathView path);
    PathResolution           ResolvePath(INode* parent, PathView path);
    PathResolution ResolvePath(INode* parent, PathView path, bool followLinks);
    PathRes        ResolvePath(DirectoryEntry* parent, PathView path,
                               bool followLinks = true);

    std::unordered_map<StringView, class Filesystem*>& GetMountPoints();

    bool MountRoot(StringView filesystemName);
    bool Mount(DirectoryEntry* parent, PathView source, PathView target,
               StringView fsName, i32 flags = 0, const void* data = nullptr);
    bool Unmount(DirectoryEntry* parent, PathView path, i32 flags = 0);

    DirectoryEntry* CreateNode(DirectoryEntry* parent, PathView path,
                               mode_t mode);
    ErrorOr<i32>    MkDir(INode* parent, mode_t mode);

    DirectoryEntry* MkNod(DirectoryEntry* parent, PathView path, mode_t mode,
                          dev_t dev);
    DirectoryEntry* Symlink(DirectoryEntry* parent, PathView path,
                            StringView target);
    DirectoryEntry* Link(DirectoryEntry* oldParent, PathView oldPath,
                         DirectoryEntry* newParent, PathView newPath,
                         i32 flags = 0);

    bool Unlink(DirectoryEntry* parent, PathView path, i32 flags = 0);
}; // namespace VFS
