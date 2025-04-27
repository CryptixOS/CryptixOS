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

class FileDescriptor;
namespace VFS
{
    Vector<std::pair<bool, StringView>>& GetFilesystems();

    INode*                               GetRootNode();
    void                                 RecursiveDelete(INode* node);

    struct PathResolution
    {
        INode* Parent   = nullptr;
        INode* Node     = nullptr;
        Path   BaseName = ""_s;
    };

    ErrorOr<FileDescriptor*> Open(INode* parent, PathView path, i32 flags,
                                  mode_t mode);

    ErrorOr<INode*>          ResolvePath(PathView path);
    PathResolution           ResolvePath(INode* parent, PathView path);
    PathResolution ResolvePath(INode* parent, PathView path, bool followLinks);

    std::unordered_map<StringView, class Filesystem*>& GetMountPoints();

    bool         MountRoot(StringView filesystemName);
    bool         Mount(INode* parent, PathView source, PathView target,
                       StringView fsName, i32 flags = 0, const void* data = nullptr);
    bool         Unmount(INode* parent, PathView path, i32 flags = 0);

    INode*       CreateNode(INode* parent, PathView path, mode_t mode);
    ErrorOr<i32> MkDir(INode* parent, mode_t mode);

    INode*       MkNod(INode* parent, PathView path, mode_t mode, dev_t dev);
    INode*       Symlink(INode* parent, PathView path, StringView target);
    INode*       Link(INode* oldParent, PathView oldPath, INode* newParent,
                      PathView newPath, i32 flags = 0);

    bool         Unlink(INode* parent, PathView path, i32 flags = 0);

}; // namespace VFS
