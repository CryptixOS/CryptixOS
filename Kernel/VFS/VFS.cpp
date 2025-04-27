/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <Arch/CPU.hpp>

#include <Library/Spinlock.hpp>
#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/EchFs/EchFs.hpp>
#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Fat32Fs/Fat32Fs.hpp>
#include <VFS/INode.hpp>
#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/VFS.hpp>

#include <cerrno>

namespace VFS
{
    static INode*                                      s_RootNode = nullptr;
    static Spinlock                                    s_Lock;
    static std::unordered_map<StringView, Filesystem*> s_MountPoints;

    Vector<std::pair<bool, StringView>>&               GetFilesystems()
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

    INode* GetRootNode()
    {
        /*Thread* thread = CPU::GetCurrentThread();
        if (!thread) return s_RootNode;

        Process* process = thread->parent;
        Assert(process);

        INode* rootNode = process->GetRootNode();
        return rootNode ? rootNode : s_RootNode;*/

        return s_RootNode;
    }

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

        if (node->IsDirectory())
            for (auto [name, child] : node->GetChildren())
                RecursiveDelete(child);

        delete node;
    }

    ErrorOr<FileDescriptor*> Open(INode* parent, PathView path, i32 flags,
                                  mode_t mode)
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

        INode* node = VFS::ResolvePath(parent, path, followSymlinks).Node;
        if (!node)
        {
            didExist = false;
            if (errno != ENOENT || !(flags & O_CREAT)) return Error(ENOENT);

            if (!parent->ValidatePermissions(current->GetCredentials(), 5))
                return Error(EACCES);
            node = VFS::CreateNode(parent, path,
                                   (mode & ~current->GetUmask()) | S_IFREG);

            if (!node) return Error(ENOENT);
        }
        else if (flags & O_EXCL) return Error(EEXIST);

        node = node->Reduce(followSymlinks, true);
        if (!node) return Error(ENOENT);

        if (node->IsSymlink()) return Error(ELOOP);
        if ((flags & O_DIRECTORY && !node->IsDirectory()))
            return Error(ENOTDIR);
        if ((node->IsDirectory() && (acc & O_WRONLY || acc & O_RDWR)))
            return Error(EISDIR);

        // TODO(v1tr10l7): check acc modes and truncate
        if (flags & O_TRUNC && node->IsRegular() && didExist)
            ;
        return new FileDescriptor(node, flags, accMode);
    }

    ErrorOr<INode*> ResolvePath(PathView path)
    {
        INode* node = ResolvePath(s_RootNode, path).Node;
        if (!node) return Error(errno);

        return node;
    }
    PathResolution ResolvePath(INode* parent, PathView path)
    {
        if (!parent || path.Absolute()) parent = GetRootNode();
        if (path.Empty())
        {
            errno = ENOENT;
            return {nullptr, nullptr, ""_sv};
        }

        auto currentNode = parent;
        if (parent != s_RootNode) parent = parent->Reduce(false, true);
        if (!currentNode->Populate()) return {nullptr, nullptr, ""_sv};

        if (path == "/"_sv || path.Empty())
            return {currentNode, currentNode, "/"_sv};

        auto getParent = [&currentNode]
        {
            if (currentNode == GetRootNode()->Reduce(false)) return currentNode;
            else if (currentNode == currentNode->GetFilesystem()->GetRootNode())
                return currentNode->GetFilesystem()
                    ->GetMountedOn()
                    ->GetParent();

            return currentNode->GetParent();
        };

        auto segments = StringView(path).Split('/');

        for (usize i = 0; i < segments.Size(); i++)
        {
            auto segment     = String(segments[i].Raw());
            bool isLast      = i == (segments.Size() - 1);

            bool previousDir = segment == ".."_sv;
            bool currentDir  = segment == "."_sv;

            currentNode      = currentNode->Reduce(false, true);

            if (currentDir || previousDir)
            {
                if (previousDir) currentNode = getParent();

                if (isLast)
                    return {getParent(), currentNode->Reduce(false, true),
                            currentNode->GetName()};

                continue;
            }

            if (currentNode->Lookup(segment))
            {
                auto node = currentNode->Lookup(segment);
                if (!node->Reduce(false, true)->Populate())
                    return {nullptr, nullptr, ""_sv};

                if (isLast)
                {
                    if (path[path.Size() - 1] == '/' && !node->IsDirectory())
                    {
                        errno = ENOTDIR;
                        return {currentNode, nullptr, ""_sv};
                    }
                    return {currentNode, node, currentNode->GetName()};
                }
                currentNode = node;

                if (currentNode->IsSymlink())
                {
                    currentNode = currentNode->Reduce(true);
                    if (!currentNode) return {nullptr, nullptr, ""_sv};
                }
                if (!currentNode->IsDirectory())
                {
                    errno = ENOTDIR;
                    return {nullptr, nullptr, ""_sv};
                }

                continue;
            }

            errno = ENOENT;
            if (isLast) return {currentNode, nullptr, Path(segment)};

            break;
        }

        errno = ENOENT;
        return {nullptr, nullptr, ""_sv};
    }
    PathResolution ResolvePath(INode* parent, PathView path, bool followLinks)
    {
        auto [p, n, b] = ResolvePath(parent, path);
        if (followLinks && n) n = n->Reduce(true);

        return {p, n, b};
    }

    std::unordered_map<StringView, class Filesystem*>& GetMountPoints()
    {
        return s_MountPoints;
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

        s_RootNode = fs->Mount(nullptr, nullptr, nullptr, "/", nullptr);

        if (s_RootNode)
            LogInfo("VFS: Mounted Filesystem '{}' on '/'", filesystemName);
        else
            LogError("VFS: Failed to mount filesystem '{}' on '/'",
                     filesystemName);

        s_MountPoints["/"] = s_RootNode->GetFilesystem();
        return s_RootNode != nullptr;
    }

    // TODO: flags
    bool Mount(INode* parent, PathView sourcePath, PathView target,
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

        INode* sourceNode = nullptr;
        if (!sourcePath.Empty())
        {
            sourceNode = ResolvePath(s_RootNode, sourcePath).Node;
            if (!sourceNode)
            {
                LogError("VFS: Failed to resolve source path -> '{}'",
                         sourcePath);
                return false;
            }
            if (sourceNode->IsDirectory())
            {
                LogError("VFS: Source node is a directory -> '{}'", sourcePath);
                return_err(false, EISDIR);
            }
        }
        auto [nparent, node, baseName] = ResolvePath(parent, target);
        bool   isRoot                  = (node == GetRootNode());
        INode* mountGate               = nullptr;

        if (!node)
        {
            LogError("VFS: Failed to resolve target path -> '{}'", target);
            goto fail;
        }

        if (!isRoot && !node->IsDirectory())
        {
            LogError("VFS: '{}' target is not a directory", target);
            return_err(false, ENOTDIR);
        }

        mountGate = fs->Mount(parent, sourceNode, node, baseName, data);
        if (!mountGate)
        {
            LogError("VFS: Failed to mount '{}' fs", fsName);
            goto fail;
        }

        node->mountGate = mountGate;

        if (sourcePath.Empty())
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName, target);
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     sourcePath, target, fsName);

        s_MountPoints[target] = fs;
        return true;
    fail:
        if (node) delete node;
        if (fs) delete fs;
        return false;
    }

    bool Unmount(INode* parent, PathView path, i32 flags)
    {
        // TODO: Unmount
        ToDo();
        return false;
    }

    INode* CreateNode(INode* parent, PathView path, mode_t mode)
    {
        ScopedLock guard(s_Lock);

        auto [newNodeParent, newNode, newNodeName] = ResolvePath(parent, path);
        if (newNode) return_err(nullptr, EEXIST);

        if (!newNodeParent) return nullptr;
        newNode = newNodeParent->GetFilesystem()->CreateNode(
            newNodeParent, StringView(newNodeName.View().Raw()), mode);
        if (newNode) newNodeParent->InsertChild(newNode, newNode->GetName());
        return newNode;
    }

    INode* MkNod(INode* parent, PathView path, mode_t mode, dev_t dev)
    {
        ScopedLock guard(s_Lock);

        auto [nparent, node, newNodeName] = ResolvePath(parent, path);
        if (node) return_err(nullptr, EEXIST);

        if (!nparent) return nullptr;
        node = nparent->GetFilesystem()->MkNod(nparent, newNodeName, mode, dev);

        if (node) nparent->InsertChild(node, node->GetName());

        return node;
    }

    INode* Symlink(INode* parent, PathView path, StringView target)
    {
        ScopedLock guard(s_Lock);

        auto [newNodeParent, newNode, newNodeName] = ResolvePath(parent, path);
        if (newNode) return_err(nullptr, EEXIST);
        if (!newNodeParent) return_err(nullptr, ENOENT);

        newNode = newNodeParent->GetFilesystem()->Symlink(newNodeParent,
                                                          newNodeName, target);
        if (newNode) newNodeParent->InsertChild(newNode, newNode->GetName());
        return newNode;
    }

    INode* Link(INode* oldParent, PathView oldPath, INode* newParent,
                PathView newPath, i32 flags)
    {
        ToDo();
        return nullptr;
    }

    bool Unlink(INode* parent, PathView path, i32 flags)
    {
        ToDo();
        return false;
    }
} // namespace VFS
