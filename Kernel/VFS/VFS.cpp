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

#include <cerrno>

namespace VFS
{
    static SpinlockProtected<FilesystemDriver::List> s_FilesystemDrivers;
    static Ref<DirectoryEntry>     s_RootDirectoryEntry = nullptr;
    static Spinlock                s_Lock;

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
        Ref  parentEntry = TryOrRet(resolver.Resolve(PathLookupFlags::eParent));

        auto maybePathRes = ResolvePath(parent, path, followSymlinks);
        RetOnError(maybePathRes);
        Ref<DirectoryEntry> dentry = maybePathRes.Value().Entry;

        if (!dentry)
        {
            didExist = false;
            if (errno != ENOENT || !(flags & O_CREAT)) return Error(ENOENT);

            if (!parent->INode()->ValidatePermissions(current->Credentials(),
                                                      5))
                return Error(EACCES);

            if (parent)
                dentry = TryOrRet(VFS::CreateFile(parent, path.BaseName(),
                                                  mode & ~current->Umask()));
            else
                dentry
                    = TryOrRet(VFS::CreateFile(path, mode & ~current->Umask()));

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
        // RetOnError(resolutionResult);
        CtosUnused(resolutionResult);

        auto parentEntry = resolver.ParentEntry();
        auto entry       = resolver.DirectoryEntry();
        if (followLinks && entry) entry = entry->FollowSymlinks().Promote();

        res.Parent   = parentEntry;
        res.Entry    = entry;
        res.BaseName = resolver.BaseName();

        return res;
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
        auto         targetEntry
            = TryOrRet(targetResolver.Resolve(PathLookupFlags::eRegular));

        bool isRoot = targetEntry == RootDirectoryEntry();
        if (!isRoot && !targetEntry->IsDirectory())
        {
            LogError("VFS: '{}' target is not a directory", target);
            return Error(ENOTDIR);
        }

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

    Ref<DirectoryEntry> CreateNode(Ref<DirectoryEntry> parent, PathView path,
                                   mode_t mode)
    {
        if (!parent) parent = RootDirectoryEntry();
        ScopedLock guard(s_Lock);

        auto       maybePathRes = ResolvePath(parent, path);
        // if (!maybePathRes&&maybePathRes.Error()) return_err(nullptr,
        // maybePathRes.Error());

        auto       pathRes      = maybePathRes.Value();

        parent                  = pathRes.Parent;
        if (pathRes.Entry) return_err(nullptr, EEXIST);

        if (!parent) return nullptr;
        auto parentINode = parent->INode();

        Ref  entry       = new DirectoryEntry(parent, pathRes.BaseName);
        auto newNodeOr
            = parentINode->Filesystem()->CreateNode(parentINode, entry, mode);
        if (!newNodeOr) return nullptr;

        // parent->InsertChild(entry);
        return entry;
    }
    ErrorOr<Ref<DirectoryEntry>> CreateFile(Ref<DirectoryEntry> directory,
                                            StringView name, mode_t mode)
    {
        Assert(directory);
        if (directory->Lookup(name)) return Error(EEXIST);

        Ref  entry          = new DirectoryEntry(directory, name);
        auto directoryINode = directory->INode();
        if (!directoryINode || !directoryINode->Filesystem())
            return Error(ENODEV);

        TryOrRet(directoryINode->CreateFile(entry, mode));
        directory->InsertChild(entry);
        entry->SetParent(directory);
        return entry;
    }
    ErrorOr<Ref<DirectoryEntry>> CreateFile(PathView path, mode_t mode)
    {
        PathResolver resolver(RootDirectoryEntry(), path);
        auto         parent = TryOrRet(resolver.Resolve(
            PathLookupFlags::eParent | PathLookupFlags::eNegativeEntry));

        return CreateFile(parent, path, mode);
    }
    ErrorOr<Ref<DirectoryEntry>> MkDir(Ref<DirectoryEntry> directory,
                                       Ref<DirectoryEntry> entry, mode_t mode)
    {
        Assert(directory);
        if (!directory->IsDirectory()) return Error(ENOTDIR);

        auto current     = Process::Current();
        auto parentINode = directory->INode();
        if (!parentINode->CanWrite(current->Credentials()))
            return Error(EACCES);

        if (directory->Lookup(entry->Name())) return Error(EEXIST);
        auto maybeEntry = parentINode->CreateDirectory(entry, mode);

        RetOnError(maybeEntry);
        return maybeEntry;
    }
    ErrorOr<Ref<DirectoryEntry>> MkDir(Ref<DirectoryEntry> directory,
                                       PathView path, mode_t mode)
    {
        auto maybePathRes = ResolvePath(directory, path, true);

        auto err          = maybePathRes.ErrorOr(EEXIST);
        if (err && err != ENOENT) return Error(err);

        Ref entry
            = new DirectoryEntry(directory, maybePathRes.Value().BaseName);
        auto maybeEntry = MkDir(directory, entry, mode);

        if (!directory) directory = maybePathRes.Value().Parent;
        // if (maybeEntry) directory->InsertChild(entry);
        return maybeEntry;
    }

    ErrorOr<Ref<DirectoryEntry>> MkNod(Ref<DirectoryEntry> parent,
                                       PathView path, mode_t mode, dev_t dev)
    {
        Assert(parent);
        auto maybePathRes = ResolvePath(parent, path);
        RetOnError(maybePathRes);

        auto pathRes = maybePathRes.Value();
        if (pathRes.Entry) return Error(EEXIST);

        Ref  entry       = new DirectoryEntry(parent, pathRes.BaseName);
        auto parentINode = parent->INode();
        auto inodeOr
            = parentINode->Filesystem()->MkNod(parentINode, entry, mode, dev);
        if (!inodeOr) return Error(inodeOr.Error());

        // if (pathRes.Parent) pathRes.Parent->InsertChild(entry);
        return entry;
    }
    ErrorOr<Ref<DirectoryEntry>> MkNod(PathView path, mode_t mode, dev_t dev)
    {
        auto maybePathRes = ResolvePath(RootDirectoryEntry(), path);
        RetOnError(maybePathRes);

        auto pathRes = maybePathRes.Value();
        auto parent  = pathRes.Parent;
        if (!parent) return Error(ENOENT);

        return MkNod(parent, pathRes.BaseName, mode, dev);
    }

    Ref<DirectoryEntry> Symlink(Ref<DirectoryEntry> parent, PathView path,
                                StringView target)
    {
        if (!parent) parent = RootDirectoryEntry();
        ScopedLock   guard(s_Lock);

        PathResolver resolver(parent, path);
        auto         directory
            = TryOrRetVal(resolver.Resolve(PathLookupFlags::eParent
                                           | PathLookupFlags::eNegativeEntry),
                          nullptr);

        auto newEntry = CreateRef<DirectoryEntry>(directory, path.BaseName());
        auto directoryINode = directory->INode();
        auto newINode       = TryOrRetVal(directoryINode->Filesystem()->Symlink(
                                        directoryINode, newEntry, target),
                                          nullptr);
        // if (resolver.ParentEntry())
        // resolver.ParentEntry()->InsertChild(newEntry);
        Assert(newINode);
        return newEntry;
    }

    Ref<DirectoryEntry> Link(Ref<DirectoryEntry> oldParent, PathView oldPath,
                             Ref<DirectoryEntry> newParent, PathView newPath,
                             i32 flags)
    {
        if (!oldParent) oldParent = RootDirectoryEntry();
        if (!newParent) newParent = RootDirectoryEntry();

        auto maybeOldPathRes = ResolvePath(oldParent, oldPath);
        auto maybeNewPathRes = ResolvePath(newParent, newPath);

        auto oldPathRes      = maybeOldPathRes.Value();
        auto newPathRes      = maybeNewPathRes.Value();

        auto oldEntry        = oldPathRes.Entry;
        Ref  newEntry        = newPathRes.Entry;
        newParent            = newPathRes.Parent;

        if (newEntry) return_err(nullptr, EEXIST);
        if (!newParent) return_err(nullptr, ENOENT);
        if (!newParent->IsDirectory()) return_err(nullptr, ENOTDIR);

        newEntry = new DirectoryEntry(newParent, newPathRes.BaseName);
        auto newParentINode = newParent->INode();

        auto maybeNewEntry  = newParentINode->Link(oldEntry, newEntry);
        if (!maybeNewEntry) return_err(nullptr, maybeNewEntry.Error());

        return maybeNewEntry.Value();
    }

    bool Unlink(Ref<DirectoryEntry> parent, PathView path, i32 flags)
    {
        if (!parent) parent = RootDirectoryEntry();

        ToDo();
        return false;
    }
} // namespace VFS
