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
    static DirectoryEntry*               s_RootNode = nullptr;

    static Spinlock                      s_Lock;

    Vector<std::pair<bool, StringView>>& GetFilesystems()
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

    DirectoryEntry* GetRootDirectoryEntry() { return s_RootNode; }
    INode* GetRootINode() { return s_RootNode ? s_RootNode->INode() : nullptr; }

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
        //      for (auto [name, child] : node->GetChildren())
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

        DirectoryEntry* dentry
            = VFS::ResolvePath(parent, path, followSymlinks).Entry;
        if (!dentry)
        {
            didExist = false;
            if (errno != ENOENT || !(flags & O_CREAT)) return Error(ENOENT);

            if (!parent->INode()->ValidatePermissions(current->GetCredentials(),
                                                      5))
                return Error(EACCES);
            dentry = VFS::CreateNode(parent, path,
                                     (mode & ~current->GetUmask()) | S_IFREG);

            if (!dentry) return Error(ENOENT);
        }
        else if (flags & O_EXCL) return Error(EEXIST);

        dentry    = dentry->FollowMounts();
        dentry    = dentry->FollowSymlinks();
        auto node = dentry->INode();
        if (!node) return Error(ENOENT);

        if (node->IsSymlink()) return Error(ELOOP);
        if ((flags & O_DIRECTORY && !node->IsDirectory()))
            return Error(ENOTDIR);
        if ((node->IsDirectory() && (acc & O_WRONLY || acc & O_RDWR)))
            return Error(EISDIR);

        // TODO(v1tr10l7): check acc modes and truncate
        if (flags & O_TRUNC && node->IsRegular() && didExist)
            ;
        return new FileDescriptor(node->DirectoryEntry(), flags, accMode);
    }

    PathResolution ResolvePath(DirectoryEntry* parent, PathView path,
                               bool followLinks)
    {
        if (!parent || path.Absolute()) parent = s_RootNode;
        PathResolution res = {nullptr, nullptr, ""_sv};

        PathWalker     resolver(parent, path);
        auto           resolutionResult = resolver.Resolve(followLinks);
        CtosUnused(resolutionResult);

        auto parentEntry = resolver.ParentEntry();
        auto entry       = resolver.DirectoryEntry();

        auto parentINode = parentEntry ? parentEntry->INode() : nullptr;
        auto inode       = entry ? entry->INode() : nullptr;

        if (followLinks && inode) inode = inode->Reduce(true);

        res.Parent   = parentINode->DirectoryEntry();
        res.Entry    = inode ? inode->DirectoryEntry() : nullptr;
        res.BaseName = resolver.BaseName();

        return res;
    }

    bool MountRoot(StringView filesystemName)
    {
        auto fs = CreateFilesystem(filesystemName, 0);
        if (!fs)
        {
            LogError("VFS: Failed to create filesystem: '{}'", filesystemName);
            return false;
        }
        if (s_RootNode)
        {
            LogError("VFS: Root already mounted!");
            return false;
        }

        s_RootNode = new DirectoryEntry("/");
        auto fsRootOr
            = fs->Mount(nullptr, nullptr, nullptr, s_RootNode, "/", nullptr);

        auto fsRoot              = fsRootOr.value();
        fsRoot->m_DirectoryEntry = s_RootNode;

        if (s_RootNode->INode())
            LogInfo("VFS: Mounted Filesystem '{}' on '/'", filesystemName);
        else
            LogError("VFS: Failed to mount filesystem '{}' on '/'",
                     filesystemName);

        MountPoint::Attach(new MountPoint(s_RootNode, fs));
        return s_RootNode != nullptr;
    }

    // TODO: flags
    bool Mount(DirectoryEntry* parent, PathView sourcePath, PathView target,
               StringView fsName, i32 flags, const void* data)
    {
        ScopedLock  guard(s_Lock);

        Filesystem* fs = CreateFilesystem(fsName, flags);
        if (!fs)
        {
            errno = ENODEV;
            LogError("VFS: Failed to create '{}' filesystem", fsName);
            return false;
        }

        DirectoryEntry* sourceEntry = nullptr;
        if (!sourcePath.Empty())
        {
            sourceEntry = ResolvePath(s_RootNode, sourcePath).Entry;
            if (!sourceEntry)
            {
                LogError("VFS: Failed to resolve source path -> '{}'",
                         sourcePath);
                return false;
            }
            if (sourceEntry->IsDirectory())
            {
                LogError("VFS: Source node is a directory -> '{}'", sourcePath);
                return_err(false, EISDIR);
            }
        }
        auto [nparent, dentry, baseName] = ResolvePath(parent, target);
        bool   isRoot                    = (dentry == GetRootDirectoryEntry());
        INode* mountGate                 = nullptr;

        if (!parent) parent = s_RootNode;
        auto   parentINode    = parent ? parent->INode() : nullptr;
        auto   sourceINode    = sourceEntry ? sourceEntry->INode() : nullptr;
        INode* targetINode    = nullptr;

        DirectoryEntry* entry = nullptr;
        MountPoint*     mountPoint = new MountPoint(dentry, fs);

        if (!dentry)
        {
            LogError("VFS: Failed to resolve target path -> '{}'", target);

            delete targetINode;
            delete fs;
            return false;
        }

        targetINode = dentry->INode();
        entry       = targetINode->DirectoryEntry();

        if (!isRoot && !dentry->IsDirectory())
        {
            LogError("VFS: '{}' target is not a directory", target);
            return_err(false, ENOTDIR);
        }

        entry            = new DirectoryEntry(parent, targetINode->GetName());
        auto mountGateOr = fs->Mount(parentINode, sourceINode, targetINode,
                                     entry, baseName, data);

        mountGate        = mountGateOr.value();
        if (!mountGate)
        {
            LogError("VFS: Failed to mount '{}' fs", fsName);
            goto fail;
        }

        dentry->m_MountGate = mountGate->DirectoryEntry();

        if (sourcePath.Empty())
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName, target);
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     sourcePath, target, fsName);

        MountPoint::Attach(mountPoint);
        return true;
    fail:
        delete targetINode;
        delete fs;
        return false;
    }

    bool Unmount(DirectoryEntry* parent, PathView path, i32 flags)
    {
        // TODO: Unmount
        ToDo();
        return false;
    }

    DirectoryEntry* CreateNode(DirectoryEntry* parentDir, PathView path,
                               mode_t mode)
    {
        if (!parentDir) parentDir = GetRootDirectoryEntry();
        ScopedLock guard(s_Lock);

        auto       pathRes     = ResolvePath(parentDir, path);
        auto       parentEntry = pathRes.Parent;
        auto       entry       = pathRes.Entry;

        if (entry) return_err(nullptr, EEXIST);

        if (!parentEntry) return nullptr;
        auto parentINode = parentEntry->INode();

        entry            = new DirectoryEntry(pathRes.BaseName);
        auto newNodeOr   = parentINode->GetFilesystem()->CreateNode(parentINode,
                                                                    entry, mode);
        auto inode       = newNodeOr.value();

        if (inode)
        {
            parentINode->InsertChild(inode, inode->GetName());
            parentEntry->InsertChild(entry);
        }
        return inode->DirectoryEntry();
    }

    DirectoryEntry* MkNod(DirectoryEntry* parent, PathView path, mode_t mode,
                          dev_t dev)
    {
        if (!parent) parent = GetRootDirectoryEntry();
        ScopedLock guard(s_Lock);

        auto       pathRes     = ResolvePath(parent, path);
        auto       parentEntry = pathRes.Parent;
        auto       entry       = pathRes.Entry;

        if (entry) return_err(nullptr, EEXIST);
        if (!parentEntry) return nullptr;

        entry            = new DirectoryEntry(pathRes.BaseName);
        auto parentINode = parentEntry->INode();
        auto inodeOr = parentINode->GetFilesystem()->MkNod(parentEntry->INode(),
                                                           entry, mode, dev);
        if (!inodeOr)
        {
            delete entry;
            return nullptr;
        }

        auto inode = inodeOr.value();
        if (inode)
        {
            parentINode->InsertChild(inode, entry->Name());
            parentEntry->InsertChild(entry);
        }

        return entry;
    }

    DirectoryEntry* Symlink(DirectoryEntry* parent, PathView path,
                            StringView target)
    {
        if (!parent) parent = GetRootDirectoryEntry();
        ScopedLock guard(s_Lock);

        auto       pathRes     = ResolvePath(parent, path);
        auto       parentEntry = pathRes.Parent;
        auto       entry       = pathRes.Entry;

        if (entry) return_err(nullptr, EEXIST);
        if (!parentEntry) return_err(nullptr, ENOENT);

        entry            = new DirectoryEntry(pathRes.BaseName);
        auto parentINode = parentEntry->INode();

        auto newNodeOr
            = parentINode->GetFilesystem()->Symlink(parentINode, entry, target);
        if (!newNodeOr) return_err(nullptr, newNodeOr.error());
        auto inode = newNodeOr.value();

        if (inode)
        {
            parentINode->InsertChild(inode, entry->Name());
            parentEntry->InsertChild(entry);
        }

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
