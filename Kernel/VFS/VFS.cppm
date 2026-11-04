/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Library/Module.hpp>
#include <Compiler.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

export module Cryptix:VFS.VFS;

export import :Debug.Assertions;
export import :Library.Logger;
export import :Library.Stacktrace;
export import :Drivers.Core.FilesystemDriver;
import :VFS.MountPoint;

export class INode;
export class DirectoryEntry;
export class MountPoint;
export class FileDescriptor;

export namespace VFS
{
    void                           Initialize();
    bool                           IsInitialized();

    ErrorOr<Ref<FilesystemDriver>> FindFilesystem(StringView name);

    ErrorOr<void>       RegisterFilesystem(Ref<FilesystemDriver> driver);
    ErrorOr<void>       UnregisterFilesystem(Ref<FilesystemDriver> driver);

    Ref<DirectoryEntry> RootDirectoryEntry();
    Ref<MountPoint>     RootMount();
    void                RecursiveDelete(INode* node);

    struct PathResolution
    {
        Ref<DirectoryEntry> Parent   = nullptr;
        Ref<DirectoryEntry> Entry    = nullptr;
        Path                BaseName = ""_s;
    };

    bool                         Access(PathView path, INodeMode mode);
    ErrorOr<Ref<DirectoryEntry>> OpenDirectoryEntry(Ref<DirectoryEntry> parent,
                                                    PathView path, isize flags,
                                                    INodeMode mode);
    ErrorOr<Ref<FileDescriptor>> Open(Ref<DirectoryEntry> parent, PathView path,
                                      isize flags, INodeMode mode);

    ErrorOr<PathResolution>      ResolvePath(Ref<DirectoryEntry> parent,
                                             PathView path, bool followLinks = true);
    ErrorOr<Ref<DirectoryEntry>> ResolveParent(Ref<DirectoryEntry> parent,
                                               PathView            path);

    ErrorOr<Ref<MountPoint>>     MountRoot(StringView filesystemName);
    ErrorOr<Ref<MountPoint>> Mount(Ref<DirectoryEntry> parent, PathView source,
                                   PathView target, StringView fsName,
                                   i32 flags = 0, const void* data = nullptr);
    bool Unmount(Ref<DirectoryEntry> parent, PathView path, i32 flags = 0);

    ErrorOr<void>                PivotRoot(PathView newRoot, PathView putOld);

    ErrorOr<void>                Sync();

    ErrorOr<Ref<DirectoryEntry>> CreateNode(Ref<DirectoryEntry> parent,
                                            StringView name, INodeMode mode,
                                            DeviceID dev,
                                            PathView target = ""_pv);
    ErrorOr<Ref<DirectoryEntry>> CreateNode(PathView path, INodeMode mode,
                                            DeviceID dev);

    ErrorOr<Ref<DirectoryEntry>> CreateFile(Ref<DirectoryEntry> directory,
                                            StringView name, INodeMode mode);
    ErrorOr<Ref<DirectoryEntry>> CreateFile(PathView path, INodeMode mode);

    ErrorOr<Ref<DirectoryEntry>> CreateDirectory(Ref<DirectoryEntry> directory,
                                                 StringView          name,
                                                 INodeMode           mode);
    ErrorOr<Ref<DirectoryEntry>> CreateDirectory(PathView path, INodeMode mode);

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

    ErrorOr<isize> SetExtendedAttribute(PathView path, StringView name,
                                        const u8* value, usize size,
                                        isize flags, bool followLinks = false);
    ErrorOr<isize> GetExtendedAttribute(PathView path, StringView name,
                                        const u8* value, usize size,
                                        bool followLinks = false);
    ErrorOr<isize> ListExtendedAttributes(PathView path, char* list, usize size,
                                          bool followLinks = false);
    ErrorOr<isize> RemoveExtendedAttribute(PathView path, StringView name,
                                           bool followLinks = false);
}; // namespace VFS
