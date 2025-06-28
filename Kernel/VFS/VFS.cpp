/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <Arch/CPU.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/EchFs/EchFs.hpp>
#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Fat32Fs/Fat32Fs.hpp>
#include <VFS/INode.hpp>
#include <VFS/MountPoint.hpp>
#include <VFS/PathWalker.hpp>
#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/VFS.hpp>

#include <cerrno>

namespace VFS
{
    static DirectoryEntry*               s_RootDirectoryEntry = nullptr;
    static Spinlock                      s_Lock;

    Vector<std::pair<bool, StringView>>& Filesystems()
    {
        static Vector<std::pair<bool, StringView>> s_Filesystems = {
            {
                false,
                "tmpfs"_sv,
            },
            {
                false,
                "devtmpfs"_sv,
            },
            {
                false,
                "procfs"_sv,
            },
            {
                true,
                "echfs"_sv,
            },
            {
                true,
                "fat32fs"_sv,
            },
            {
                true,
                "ext2fs"_sv,
            },
        };

        return s_Filesystems;
    }

    DirectoryEntry*    GetRootDirectoryEntry() { return s_RootDirectoryEntry; }

    static Filesystem* CreateFilesystem(StringView name, u32 flags)
    {
        Filesystem* fs = nullptr;
        if (name == "tmpfs") fs = new TmpFs(flags);
        else if (name == "devtmpfs") fs = new DevTmpFs(flags);
        else if (name == "procfs") fs = new ProcFs(flags);
        else if (name == "echfs") fs = new EchFs(flags);
        else if (name == "fat32fs") fs = new Fat32Fs(flags);
        else if (name == "ext2fs") fs = new Ext2Fs(flags);

        if (!fs) LogError("VFS: No filesystem driver found for '{}'", name);
        return fs;
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

    ErrorOr<FileDescriptor*> Open(DirectoryEntry* parent, PathView path,
                                  i32 flags, mode_t mode)
    {
        Process* current        = Process::GetCurrent();
        bool     followSymlinks = !(flags & O_NOFOLLOW);
        auto     acc            = flags & O_ACCMODE;
        mode &= ~(S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX | S_ISUID | S_ISGID);

        // if (flags & ~VALID_OPEN_FLAGS || flags & (O_CREAT | O_DIRECTORY))
        //     return Error(EINVAL);

        bool           didExist = true;

        FileAccessMode accMode  = FileAccessMode::eNone;
        switch (acc)
        {
            case O_RDONLY: accMode |= FileAccessMode::eRead; break;
            case O_WRONLY: accMode |= FileAccessMode::eWrite; break;
            case O_RDWR:
                accMode |= FileAccessMode::eRead | FileAccessMode::eWrite;
                break;

            default: return Error(EINVAL);
        }

        if (flags & O_TMPFILE)
        {
            if (!(flags & O_DIRECTORY)) return Error(EINVAL);
            if (!(accMode & FileAccessMode::eWrite)) return Error(EINVAL);
            return Error(ENOSYS);
        }
        if (flags & O_PATH)
        {
            if (flags & ~(O_DIRECTORY | O_NOFOLLOW | O_PATH | O_CLOEXEC))
                return Error(EINVAL);
            accMode = FileAccessMode::eNone;
        }

        auto maybePathRes = ResolvePath(parent, path, followSymlinks);
        RetOnError(maybePathRes);
        DirectoryEntry* dentry = maybePathRes.value().Entry;

        if (!dentry)
        {
            didExist = false;
            if (errno != ENOENT || !(flags & O_CREAT)) return Error(ENOENT);

            if (!parent->INode()->ValidatePermissions(current->Credentials(),
                                                      5))
                return Error(EACCES);
            dentry = VFS::CreateNode(parent, path,
                                     (mode & ~current->Umask()) | S_IFREG);

            if (!dentry) return Error(ENOENT);
        }
        else if (flags & O_EXCL) return Error(EEXIST);

        dentry = dentry->FollowMounts();
        dentry = dentry->FollowSymlinks();

        if (dentry->IsSymlink()) return Error(ELOOP);
        if ((flags & O_DIRECTORY && !dentry->IsDirectory()))
            return Error(ENOTDIR);
        if ((dentry->IsDirectory() && (acc & O_WRONLY || acc & O_RDWR)))
            return Error(EISDIR);

        // TODO(v1tr10l7): check acc modes and truncate
        if (flags & O_TRUNC && dentry->IsRegular() && didExist)
            ;
        return new FileDescriptor(dentry, flags, accMode);
    }

    ErrorOr<PathResolution> ResolvePath(DirectoryEntry* parent, PathView path,
                                        bool followLinks)
    {
        if (!parent || path.Absolute()) parent = s_RootDirectoryEntry;
        PathResolution res = {nullptr, nullptr, ""_sv};

        PathWalker     resolver(parent, path);
        auto           resolutionResult = resolver.Resolve(followLinks);
        // RetOnError(resolutionResult);
        CtosUnused(resolutionResult);

        auto parentEntry = resolver.ParentEntry();
        auto entry       = resolver.DirectoryEntry();
        if (followLinks && entry) entry = entry->FollowSymlinks();

        res.Parent   = parentEntry;
        res.Entry    = entry;
        res.BaseName = resolver.BaseName();

        return res;
    }

    ErrorOr<MountPoint*> MountRoot(StringView filesystemName)
    {
        auto fs = CreateFilesystem(filesystemName, 0);
        if (!fs)
        {
            LogError("VFS: Failed to create filesystem: '{}'", filesystemName);
            return Error(ENODEV);
        }
        if (s_RootDirectoryEntry)
        {
            LogError("VFS: Root already mounted!");

            delete fs;
            return Error(EEXIST);
        }

        auto maybeFsRoot = fs->Mount("", nullptr);
        if (!maybeFsRoot)
        {
            delete fs;
            delete s_RootDirectoryEntry;

            return Error(ENODEV);
        }

        s_RootDirectoryEntry = maybeFsRoot.value();
        if (s_RootDirectoryEntry)
            LogInfo("VFS: Mounted Filesystem '{}' on '/'", filesystemName);
        else
            LogError("VFS: Failed to mount filesystem '{}' on '/'",
                     filesystemName);

        auto rootMountPoint = new MountPoint(s_RootDirectoryEntry, fs);
        MountPoint::Attach(rootMountPoint);
        return rootMountPoint;
    }

    // TODO: flags
    ErrorOr<MountPoint*> Mount(DirectoryEntry* parent, PathView sourcePath,
                               PathView target, StringView fsName, i32 flags,
                               const void* data)
    {
        if (target.Empty()) return Error(EINVAL);

        Filesystem* fs = CreateFilesystem(fsName, flags);
        if (!fs)
        {
            LogError("VFS: Failed to create '{}' filesystem", fsName);
            return Error(ENODEV);
        }

        auto maybePathRes = ResolvePath(parent, target);
        RetOnError(maybePathRes);

        auto [targetParent, targetEntry, targetName] = maybePathRes.value();
        bool            isRoot    = (targetEntry == GetRootDirectoryEntry());
        DirectoryEntry* mountRoot = nullptr;

        parent                    = targetParent;
        if (!parent) parent = s_RootDirectoryEntry;
        INode*      targetINode = nullptr;
        MountPoint* mountPoint  = nullptr;

        if (!targetEntry)
        {
            LogError("VFS: Failed to resolve target path -> '{}'", target);

            delete targetINode;
            delete fs;
            return Error(ENOENT);
        }

        targetINode = targetEntry->INode();
        if (!isRoot && !targetEntry->IsDirectory())
        {
            LogError("VFS: '{}' target is not a directory", target);
            delete fs;
            delete targetINode;
            return Error(ENOTDIR);
        }

        mountPoint       = new MountPoint(targetEntry, fs);
        auto mountGateOr = fs->Mount(sourcePath, data);
        mountRoot        = mountGateOr.value();

        if (!mountRoot)
        {
            LogError("VFS: Failed to mount '{}' fs", fsName);
            goto fail;
        }

        if (targetEntry)
        {
            mountRoot->SetParent(targetEntry);
            targetEntry->SetMountGate(mountRoot->INode(), mountRoot);
        }
        targetEntry->m_MountGate = mountRoot;

        if (sourcePath.Empty())
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName, target);
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     sourcePath, target, fsName);

        MountPoint::Attach(mountPoint);
        return mountPoint;
    fail:
        delete targetINode;
        delete fs;
        return Error(ENODEV);
    }

    bool Unmount(DirectoryEntry* parent, PathView path, i32 flags)
    {
        // TODO: Unmount
        ToDo();
        return false;
    }

    DirectoryEntry* CreateNode(DirectoryEntry* parent, PathView path,
                               mode_t mode)
    {
        if (!parent) parent = GetRootDirectoryEntry();
        ScopedLock guard(s_Lock);

        auto       maybePathRes = ResolvePath(parent, path);
        // if (!maybePathRes&&maybePathRes.error()) return_err(nullptr,
        // maybePathRes.error());

        auto       pathRes      = maybePathRes.value();

        parent                  = pathRes.Parent;
        auto entry              = pathRes.Entry;

        if (entry) return_err(nullptr, EEXIST);

        if (!parent) return nullptr;
        auto parentINode = parent->INode();

        entry            = new DirectoryEntry(parent, pathRes.BaseName);
        auto newNodeOr
            = parentINode->Filesystem()->CreateNode(parentINode, entry, mode);
        if (!newNodeOr)
        {
            delete entry;
            return nullptr;
        }
        return entry;
    }
    static ErrorOr<DirectoryEntry*> CreateFile(DirectoryEntry* parent,
                                               StringView name, mode_t mode)
    {
        Assert(parent);
        auto maybePathRes = ResolvePath(parent, name);
        // RetOnError(maybePathRes);

        auto pathRes      = maybePathRes.value();
        if (pathRes.Entry) return Error(EEXIST);

        auto entry       = new DirectoryEntry(parent, name);
        auto parentINode = parent->INode();
        auto inodeOr
            = parentINode->Filesystem()->CreateNode(parentINode, entry, mode);

        if (!inodeOr)
        {
            delete entry;
            return Error(inodeOr.error());
        }

        return entry;
    }
    ErrorOr<DirectoryEntry*> CreateFile(PathView path, mode_t mode)
    {
        PathWalker resolver(GetRootDirectoryEntry(), path);
        auto       maybeEntry = resolver.Resolve();
        if (!maybeEntry && maybeEntry.error() != ENOENT)
            return Error(maybeEntry.error());

        auto parent = resolver.ParentEntry();
        if (!parent) return Error(ENODEV);

        return CreateFile(parent, path, mode);
    }
    ErrorOr<DirectoryEntry*> MkDir(DirectoryEntry* directory,
                                   DirectoryEntry* entry, mode_t mode)
    {
        Assert(directory);
        if (!directory->IsDirectory()) return Error(ENOTDIR);

        auto current     = Process::Current();
        auto parentINode = directory->INode();
        if (!parentINode->CanWrite(current->Credentials()))
            return Error(EACCES);

        if (directory->Lookup(entry->Name())) return Error(EEXIST);
        auto filesystem = directory->INode()->Filesystem();
        auto inodeOr = filesystem->CreateNode(parentINode, entry, mode, 0, 0);

        RetOnError(inodeOr);
        return entry;
    }
    ErrorOr<DirectoryEntry*> MkDir(DirectoryEntry* directory, PathView path,
                                   mode_t mode)
    {
        auto maybePathRes = ResolvePath(directory, path, true);
        auto err          = maybePathRes.error_or(EEXIST);
        if (err && err != ENOENT) return Error(err);

        auto entry
            = new DirectoryEntry(directory, maybePathRes.value().BaseName);
        auto maybeEntry = MkDir(directory, entry, mode);

        if (!maybeEntry) delete entry;
        return entry;
    }

    ErrorOr<DirectoryEntry*> MkNod(DirectoryEntry* parent, PathView path,
                                   mode_t mode, dev_t dev)
    {
        Assert(parent);
        auto maybePathRes = ResolvePath(parent, path);
        RetOnError(maybePathRes);

        auto pathRes = maybePathRes.value();
        if (pathRes.Entry) return Error(EEXIST);

        auto entry       = new DirectoryEntry(parent, pathRes.BaseName);
        auto parentINode = parent->INode();
        auto inodeOr
            = parentINode->Filesystem()->MkNod(parentINode, entry, mode, dev);
        if (!inodeOr)
        {
            delete entry;
            return Error(inodeOr.error());
        }

        return entry;
    }
    ErrorOr<DirectoryEntry*> MkNod(PathView path, mode_t mode, dev_t dev)
    {
        auto maybePathRes = ResolvePath(GetRootDirectoryEntry(), path);
        RetOnError(maybePathRes);

        auto pathRes = maybePathRes.value();
        auto parent  = pathRes.Parent;
        if (!parent) return Error(ENOENT);

        return MkNod(parent, pathRes.BaseName, mode, dev);
    }

    DirectoryEntry* Symlink(DirectoryEntry* parent, PathView path,
                            StringView target)
    {
        if (!parent) parent = GetRootDirectoryEntry();
        ScopedLock guard(s_Lock);

        auto       maybePathRes = ResolvePath(parent, path);
        if (!maybePathRes) return_err(nullptr, maybePathRes.error());

        auto pathRes = maybePathRes.value();
        parent       = pathRes.Parent;
        auto entry   = pathRes.Entry;

        if (entry) return_err(nullptr, EEXIST);
        if (!parent) return_err(nullptr, ENOENT);

        entry            = new DirectoryEntry(parent, pathRes.BaseName);
        auto parentINode = parent->INode();

        auto newNodeOr
            = parentINode->Filesystem()->Symlink(parentINode, entry, target);
        if (!newNodeOr) return_err(nullptr, newNodeOr.error());

        return entry;
    }

    DirectoryEntry* Link(DirectoryEntry* oldParent, PathView oldPath,
                         DirectoryEntry* newParent, PathView newPath, i32 flags)
    {
        ToDo();
        return nullptr;
    }

    bool Unlink(DirectoryEntry* parent, PathView path, i32 flags)
    {
        if (!parent) parent = GetRootDirectoryEntry();

        ToDo();
        return false;
    }
} // namespace VFS
