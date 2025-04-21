/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "nostd/string.hpp"
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
                "tmpfs",
            },
            {
                false,
                "devtmpfs",
            },
            {
                false,
                "procfs",
            },
            {
                true,
                "echfs",
            },
            {
                true,
                "fat32fs",
            },
            {
                true,
                "ext2fs",
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

    static Filesystem* CreateFilesystem(std::string_view name, u32 flags)
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

    std::expected<FileDescriptor*, std::errno_t>
    Open(INode* parent, PathView path, i32 flags, mode_t mode)
    {
        Process* current        = Process::GetCurrent();
        bool     followSymlinks = !(flags & O_NOFOLLOW);
        auto     acc            = flags & O_ACCMODE;
        mode &= ~(S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX | S_ISUID | S_ISGID);

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

        if (flags & O_TMPFILE && !(accMode & FileAccessMode::eWrite))
            return Error(EINVAL);

        INode* node
            = std::get<1>(VFS::ResolvePath(parent, path, followSymlinks));
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

        if (flags & O_EXCL && didExist) return Error(EEXIST);
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
        INode* node = std::get<1>(ResolvePath(s_RootNode, path));
        if (!node) return Error(errno);

        return node;
    }
    std::tuple<INode*, INode*, std::string> ResolvePath(INode*   parent,
                                                        PathView path)
    {
        if (!parent || path.IsAbsolute()) parent = GetRootNode();
        if (path.IsEmpty())
        {
            errno = ENOENT;
            return {nullptr, nullptr, ""};
        }

        auto currentNode = parent;
        if (parent != s_RootNode) parent = parent->Reduce(false, true);
        if (!currentNode->Populate()) return {nullptr, nullptr, ""};

        if (path == "/" || path.IsEmpty())
            return {currentNode, currentNode, "/"};

        auto getParent = [&currentNode]
        {
            if (currentNode == GetRootNode()->Reduce(false)) return currentNode;
            else if (currentNode == currentNode->GetFilesystem()->GetRootNode())
                return currentNode->GetFilesystem()
                    ->GetMountedOn()
                    ->GetParent();

            return currentNode->GetParent();
        };

        auto segments = path.SplitPath();

        for (usize i = 0; i < segments.Size(); i++)
        {
            auto segment     = segments[i];
            bool isLast      = i == (segments.Size() - 1);

            bool previousDir = segment == "..";
            bool currentDir  = segment == ".";

            currentNode      = currentNode->Reduce(false, true);

            if (currentDir || previousDir)
            {
                if (previousDir) currentNode = getParent();

                if (isLast)
                    return {getParent(), currentNode->Reduce(false, true),
                            currentNode->GetName()};

                continue;
            }

            if (currentNode->GetChildren().contains(segment))
            {
                auto node = currentNode->GetChildren()[segment];
                if (!node->Reduce(false, true)->Populate())
                    return {nullptr, nullptr, ""};

                if (isLast)
                {
                    if (path[path.GetSize() - 1] == '/' && !node->IsDirectory())
                    {
                        errno = ENOTDIR;
                        return {currentNode, nullptr, ""};
                    }
                    return {currentNode, node, node->GetName()};
                }
                currentNode = node;

                if (currentNode->IsSymlink())
                {
                    currentNode = currentNode->Reduce(true);
                    if (!currentNode) return {nullptr, nullptr, ""};
                }
                if (!currentNode->IsDirectory())
                {
                    errno = ENOTDIR;
                    return {nullptr, nullptr, ""};
                }

                continue;
            }

            errno = ENOENT;
            if (isLast)
                return {currentNode, nullptr, {segment.data(), segment.size()}};

            break;
        }

        errno = ENOENT;
        return {nullptr, nullptr, ""};
    }
    std::tuple<INode*, INode*, std::string>
    ResolvePath(INode* parent, PathView path, bool followLinks)
    {
        auto [p, n, b] = ResolvePath(parent, path);
        if (followLinks && n) n = n->Reduce(true);

        return {p, n, b};
    }

    std::unordered_map<StringView, class Filesystem*>& GetMountPoints()
    {
        return s_MountPoints;
    }

    bool MountRoot(std::string_view filesystemName)
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
            LogInfo("VFS: Mounted Filesystem '{}' on '/'",
                    filesystemName.data());
        else
            LogError("VFS: Failed to mount filesystem '{}' on '/'",
                     filesystemName.data());

        s_MountPoints["/"] = s_RootNode->GetFilesystem();
        return s_RootNode != nullptr;
    }

    // TODO: flags
    bool Mount(INode* parent, PathView sourcePath, PathView target,
               std::string_view fsName, i32 flags, const void* data)
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
        if (!sourcePath.IsEmpty())
        {
            sourceNode = std::get<1>(ResolvePath(s_RootNode, sourcePath));
            if (!sourceNode)
            {
                LogError("VFS: Failed to resolve source path -> '{}'",
                         sourcePath.Raw());
                return false;
            }
            if (sourceNode->IsDirectory())
            {
                LogError("VFS: Source node is a directory -> '{}'",
                         sourcePath.Raw());
                return_err(false, EISDIR);
            }
        }
        auto [nparent, node, basename] = ResolvePath(parent, target);
        bool   isRoot                  = (node == GetRootNode());
        INode* mountGate               = nullptr;

        if (!node)
        {
            LogError("VFS: Failed to resolve target path -> '{}'",
                     target.Raw());
            goto fail;
        }

        if (!isRoot && !node->IsDirectory())
        {
            LogError("VFS: '{}' target is not a directory", target.Raw());
            return_err(false, ENOTDIR);
        }

        mountGate = fs->Mount(parent, sourceNode, node, basename, data);
        if (!mountGate)
        {
            LogError("VFS: Failed to mount '{}' fs", fsName);
            goto fail;
        }

        node->mountGate = mountGate;

        if (sourcePath.IsEmpty())
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName.data(),
                     target.Raw());
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     sourcePath.Raw(), target.Raw(), fsName.data());

        s_MountPoints[target.Raw()] = fs;
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

        // LogInfo("Creating path: {}", path);
        auto [newNodeParent, newNode, newNodeName] = ResolvePath(parent, path);
        // LogInfo("Creating node: {}", name);
        if (newNode) return_err(nullptr, EEXIST);

        if (!newNodeParent) return nullptr;
        newNode = newNodeParent->GetFilesystem()->CreateNode(newNodeParent,
                                                             newNodeName, mode);
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

    INode* Symlink(INode* parent, PathView path, std::string_view target)
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
