/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <Arch/CPU.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Library/Locking/SpinlockProtected.hpp>
#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/EchFs/EchFs.hpp>
#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Fat32Fs/Fat32Fs.hpp>
#include <VFS/INode.hpp>
#include <VFS/MountPoint.hpp>
#include <VFS/PathResolver.hpp>
#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/VFS.hpp>

namespace VFS
{
    static SpinlockProtected<FilesystemDriver::List> s_FilesystemDrivers;
    static Ref<DirectoryEntry>     s_RootDirectoryEntry = nullptr;

    ErrorOr<Ref<FilesystemDriver>> FindFilesystemDriver(StringView name)
    {
        Ref<FilesystemDriver> found = nullptr;
        s_FilesystemDrivers.ForEachRead(
            [&](auto fs)
            {
                if (fs->FilesystemName == name) found = fs;
            });

        if (!found) return Error(ENODEV);
        return found;
    }

    ErrorOr<void> RegisterFilesystem(Ref<FilesystemDriver> driver)
    {
        Assert(driver);

        StringView fsName = driver->FilesystemName;
        LogTrace("VFS: Trying to register a filesystem driver: '{}'", fsName);
        if (driver->Hook.IsLinked())
        {
            LogError("VFS: The driver: '{}' is already linked!", fsName);
            return Error(EBUSY);
        }

        auto found = FindFilesystemDriver(driver->FilesystemName);
        if (found)
        {
            LogError("VFS: Driver with name: '{}', already exists!", fsName);
            return Error(EEXIST);
        }

        s_FilesystemDrivers.With([&](auto& list) { list.PushBack(driver); });

        LogTrace("VFS: Successfully registered '{}' driver", fsName);
        return {};
    }
    ErrorOr<void> UnregisterFilesystem(Ref<FilesystemDriver> driver)
    {
        Assert(driver);
        Assert(driver->Hook.IsLinked());

        auto fsName = driver->FilesystemName;
        LogTrace("VFS: Trying to unregister filesystem driver for '{}'",
                 fsName);

        auto fs = TryOrRetFmt(FindFilesystemDriver(driver->FilesystemName),
                              Error(result.Error()),
                              "VFS: Failed to find the driver '{}'", fsName);

        fs->Hook.Unlink(fs);
        return {};
    }

    static ErrorOr<Ref<Filesystem>> InstantiateFilesystem(StringView name)
    {
        Ref<FilesystemDriver> fsDriver = TryOrRet(FindFilesystemDriver(name));

        LogTrace(
            "VFS: Successfully retrieved '{}' file system driver, needed "
            "for mounting",
            name);
        if (!fsDriver->Instantiate)
        {
            LogError("VFS: The '{}' driver's Instantiate function is nullptr",
                     name);
            return Error(ENODEV);
        }

        LogTrace("VFS: Instantiating '{}' instance...", name);
        Ref<Filesystem> fs = TryOrRet(fsDriver->Instantiate());

        if (!fs)
        {
            LogError("VFS: Failed to create filesystem: '{}'", name);
            return Error(ENODEV);
        }

        return fs;
    }

    Ref<DirectoryEntry> RootDirectoryEntry()
    {
        auto root = s_RootDirectoryEntry;

        return root->FollowMounts().Promote();
    }

    void RecursiveDelete(INode* node)
    {
        if (!node) return;

        // TODO(v1tr10l7): Recursive deletion of inodes
        //  if (node->IsDirectory())
        //      for (auto [name, child] : node->Children())
        //          RecursiveDelete(child);

        delete node;
    }

    ErrorOr<Ref<DirectoryEntry>> OpenDirectoryEntry(Ref<DirectoryEntry> parent,
                                                    PathView path, isize flags,
                                                    mode_t mode)
    {
        Process* current        = Process::GetCurrent();
        bool     followSymlinks = !(flags & O_NOFOLLOW);
        auto     acc            = flags & O_ACCMODE;
        mode &= ~(S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX | S_ISUID | S_ISGID);

        // if (flags & ~VALID_OPEN_FLAGS || flags & (O_CREAT | O_DIRECTORY))
        //     return Error(EINVAL);

        bool didExist = true;

        if (flags & O_TMPFILE)
        {
            if (!(flags & O_DIRECTORY)) return Error(EINVAL);
            if (acc != O_WRONLY && acc != O_RDWR) return Error(EINVAL);
            return Error(ENOSYS);
        }
        if (flags & O_PATH)
        {
            if (flags & ~(O_DIRECTORY | O_NOFOLLOW | O_PATH | O_CLOEXEC))
                return Error(EINVAL);
        }

        PathResolver resolver(parent, path);
        Ref  directory = TryOrRet(resolver.Resolve(PathLookupFlags::eParent));

        auto maybePathRes = ResolvePath(parent, path, followSymlinks);
        RetOnError(maybePathRes);
        Ref<DirectoryEntry> dentry = maybePathRes.Value().Entry;

        if (!dentry)
        {
            didExist = false;
            if (errno != ENOENT || !(flags & O_CREAT)) return Error(ENOENT);

            if (!directory->INode()->ValidatePermissions(current->Credentials(),
                                                         5))
                return Error(EACCES);

            dentry = TryOrRet(VFS::CreateFile(directory, path.BaseName(),
                                              mode & ~current->Umask()));

            if (!dentry) return Error(ENOENT);
        }
        else if (flags & O_EXCL) return Error(EEXIST);

        dentry = dentry->FollowMounts()->FollowSymlinks().Promote();

        if (dentry->IsSymlink()) return Error(ELOOP);
        if ((flags & O_DIRECTORY && !dentry->IsDirectory()))
            return Error(ENOTDIR);
        if ((dentry->IsDirectory() && (acc & O_WRONLY || acc & O_RDWR)))
            return Error(EISDIR);

        // TODO(v1tr10l7): check acc modes and truncate
        if (flags & O_TRUNC && dentry->IsRegular() && didExist)
            ;
        return dentry;
    }
    ErrorOr<FileDescriptor*> Open(Ref<DirectoryEntry> parent, PathView path,
                                  isize flags, mode_t mode)
    {
        auto maybeEntry = OpenDirectoryEntry(parent, path, flags, mode);
        RetOnError(maybeEntry);

        auto           acc     = flags & O_ACCMODE;
        FileAccessMode accMode = FileAccessMode::eNone;
        switch (acc)
        {
            case O_RDONLY: accMode |= FileAccessMode::eRead; break;
            case O_WRONLY: accMode |= FileAccessMode::eWrite; break;
            case O_RDWR:
                accMode |= FileAccessMode::eRead | FileAccessMode::eWrite;
                break;

            default: return Error(EINVAL);
        }
        if (flags & O_PATH) accMode = FileAccessMode::eNone;

        auto dentry = maybeEntry.Value();
        return new FileDescriptor(dentry, flags, accMode);
    }

    ErrorOr<PathResolution> ResolvePath(Ref<DirectoryEntry> parent,
                                        PathView path, bool followLinks)
    {
        if (!parent || path.Absolute()) parent = RootDirectoryEntry();
        PathResolution res = {nullptr, nullptr, ""_sv};

        PathResolver   resolver(parent, path);
        auto           resolutionResult = resolver.Resolve(followLinks);
        CtosUnused(resolutionResult);

        auto parentEntry = resolver.ParentEntry();
        auto entry       = resolver.DirectoryEntry();
        if (followLinks && entry) entry = entry->FollowSymlinks().Promote();

        res.Parent   = parentEntry;
        res.Entry    = entry;
        res.BaseName = resolver.BaseName();

        return res;
    }
    ErrorOr<Ref<DirectoryEntry>> ResolveParent(Ref<DirectoryEntry> parent,
                                               PathView            path)
    {
        PathResolver resolver(RootDirectoryEntry(), path);
        auto directory = TryOrRet(resolver.Resolve(PathLookupFlags::eParent));
        if (directory->IsMountPoint())
            directory = directory->FollowMounts().Promote();

        return directory;
    }

    ErrorOr<Ref<MountPoint>> MountRoot(StringView filesystemName)
    {
        if (s_RootDirectoryEntry)
        {
            LogError("VFS: Root already mounted!");
            return Error(EEXIST);
        }

        auto root = s_RootDirectoryEntry = CreateRef<DirectoryEntry>("/");
        if (!root) return Error(ENOMEM);

        return TryOrRet(Mount(nullptr, ""_pv, "/"_pv, filesystemName));
    }

    // TODO: flags
    ErrorOr<Ref<MountPoint>> Mount(Ref<DirectoryEntry> parent,
                                   PathView sourcePath, PathView target,
                                   StringView fsName, i32 flags,
                                   const void* data)
    {
        Ref<Filesystem> fs = TryOrRet(InstantiateFilesystem(fsName));
        if (target.Empty()) return Error(EINVAL);

        PathResolver targetResolver(parent, target);
        auto         targetEntry = TryOrRet(targetResolver.Resolve(
            PathLookupFlags::eRegular | PathLookupFlags::eMountPoint));

        bool         isRoot      = targetEntry == RootDirectoryEntry();
        if (!isRoot && !targetEntry->IsDirectory())
        {
            LogError("VFS: '{}' target is not a directory", target);
            return Error(ENOTDIR);
        }

        if (targetEntry->IsMountPoint() || MountPoint::Lookup(targetEntry))
            return Error(EBUSY);
        Ref<MountPoint> mountPoint = CreateRef<MountPoint>(targetEntry, fs);
        auto            fsRoot     = TryOrRetFmt(
            fs->Mount(sourcePath, data), Error(result.Error()),
            "VFS: Failed to mount filesystem '{}' on '/'", fsName);

        if (!fsRoot)
        {
            LogError("VFS: Failed to mount '{}' fs", fsName);
            return Error(ENODEV);
        }

        fsRoot->SetParent(targetEntry);
        targetEntry->SetMountGate(fsRoot);

        if (sourcePath.Empty())
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName, target);
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     sourcePath, target, fsName);

        MountPoint::Attach(mountPoint);
        return mountPoint;
    }

    bool Unmount(Ref<DirectoryEntry> parent, PathView path, i32 flags)
    {
        // TODO: Unmount
        ToDo();
        return false;
    }

    ErrorOr<Ref<DirectoryEntry>> CreateNode(Ref<DirectoryEntry> directory,
                                            StringView name, mode_t mode,
                                            dev_t dev, PathView target)
    {
        Assert(directory);
        if (!directory->IsDirectory()) return Error(ENOTDIR);
        if (directory->Lookup(name)) return Error(EEXIST);

        Ref  entry          = CreateRef<DirectoryEntry>(nullptr, name);
        auto directoryINode = directory->INode();
        if (!directoryINode || !directoryINode->Filesystem())
            return Error(ENODEV);

        switch (mode & S_IFMT)
        {
            case S_IFREG:
                TryOrRet(directoryINode->CreateFile(entry, mode));
                break;
            case S_IFDIR:
                TryOrRet(directoryINode->CreateDirectory(entry, mode));
                break;
            case S_IFLNK:
                TryOrRet(directoryINode->Symlink(entry, target));
                break;

            default:
                TryOrRet(directoryINode->CreateNode(entry, mode, dev));
                break;
        }

        directory->InsertChild(entry);
        entry->SetParent(directory);
        return entry;
    }
    ErrorOr<Ref<DirectoryEntry>> CreateNode(PathView path, mode_t mode,
                                            dev_t dev)
    {
        return CreateNode(TryOrRet(ResolveParent(RootDirectoryEntry(), path)),
                          path.BaseName(), mode, dev);
    }

    ErrorOr<Ref<DirectoryEntry>> CreateFile(Ref<DirectoryEntry> directory,
                                            StringView name, mode_t mode)
    {
        mode &= S_IFMT;
        mode |= S_IFREG;

        return CreateNode(directory, name, mode, 0);
    }
    ErrorOr<Ref<DirectoryEntry>> CreateFile(PathView path, mode_t mode)
    {
        return CreateFile(TryOrRet(ResolveParent(RootDirectoryEntry(), path)),
                          path.BaseName(), mode);
    }

    ErrorOr<Ref<DirectoryEntry>> CreateDirectory(Ref<DirectoryEntry> directory,
                                                 StringView name, mode_t mode)
    {
        mode &= S_IFMT;
        mode |= S_IFDIR;

        return CreateNode(directory, name, mode, 0);
    }
    ErrorOr<Ref<DirectoryEntry>> CreateDirectory(PathView path, mode_t mode)
    {
        return CreateDirectory(
            TryOrRet(ResolveParent(RootDirectoryEntry(), path)),
            path.BaseName(), mode);
    }

    ErrorOr<Ref<DirectoryEntry>> Symlink(Ref<DirectoryEntry> directory,
                                         StringView name, PathView targetPath)
    {
        mode_t mode = 0777 | S_IFLNK;

        return CreateNode(directory, name, mode, 0, targetPath);
    }
    ErrorOr<Ref<DirectoryEntry>> Symlink(PathView path, PathView targetPath)
    {
        return Symlink(TryOrRet(ResolveParent(RootDirectoryEntry(), path)),
                       path.BaseName(), targetPath);
    }

    ErrorOr<Ref<DirectoryEntry>> Link(Ref<DirectoryEntry> oldParent,
                                      StringView          oldName,
                                      Ref<DirectoryEntry> newParent,
                                      StringView newName, i32 flags)
    {
        Assert(oldParent);
        Assert(newParent);

        auto oldEntry = oldParent->Lookup(oldName);
        if (!oldEntry) return Error(ENOENT);
        if (oldEntry->IsDirectory()) return Error(EPERM);

        auto newEntry = newParent->Lookup(oldName);
        if (newEntry) return Error(EEXIST);

        newEntry            = CreateRef<DirectoryEntry>(nullptr, newName);

        auto newParentINode = newParent->INode();
        TryOrRet(newParentINode->Link(oldEntry, newEntry));

        newParent->InsertChild(newEntry);
        newEntry->SetParent(newParent);

        LogInfo("Link result => oldEntry: `{}`, newEntry: `{}`",
                oldEntry->Path(), newEntry->Path());
        LogInfo("old parent name: `{}`, new parent name: `{}`",
                oldParent->Name(), newParent->Name());
        LogInfo("old parent path: `{}`, new parent path: `{}`",
                oldParent->Path(), newParent->Path());
        LogInfo("old name: `{}`, new name: `{}`", oldName, newName);
        return newEntry;
    }
    ErrorOr<Ref<DirectoryEntry>> Link(PathView oldPath, PathView newPath,
                                      i32 flags)
    {
        auto oldParent = TryOrRet(ResolveParent(RootDirectoryEntry(), oldPath));
        auto newParent = TryOrRet(ResolveParent(RootDirectoryEntry(), newPath));

        return Link(oldParent, oldPath.BaseName(), newParent,
                    newPath.BaseName(), flags);
    }

    bool Unlink(Ref<DirectoryEntry> parent, PathView path, i32 flags)
    {
        if (!parent)
            parent = path.StartsWith("/") ? RootDirectoryEntry()
                                          : Process::Current()->CWD();

        ToDo();
        return false;
    }
} // namespace VFS
