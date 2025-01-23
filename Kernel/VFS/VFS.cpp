
/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>
#include <Utility/Spinlock.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/INode.hpp>
#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/VFS.hpp>

#include <cerrno>

namespace VFS
{
    static INode*   s_RootNode = nullptr;
    static Spinlock s_Lock;
    static std::unordered_map<std::string_view, Filesystem*> s_MountPoints;

    INode*                                                   GetRootNode()
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
        Process* current        = CPU::GetCurrentThread()->parent;
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

            default: return std::unexpected(EINVAL);
        }

        if (flags & O_TMPFILE && !(accMode & FileAccessMode::eWrite))
            return std::unexpected(EINVAL);

        INode* node
            = std::get<1>(VFS::ResolvePath(parent, path, followSymlinks));
        if (!node)
        {
            didExist = false;
            if (errno != ENOENT || !(flags & O_CREAT)) return nullptr;

            if (!parent->ValidatePermissions(current->GetCredentials(), 5))
                return std::unexpected(EACCES);
            node = VFS::CreateNode(parent, path,
                                   (mode & ~current->GetUMask()) | S_IFREG);

            if (!node) return std::unexpected(ENOENT);
        }

        if (flags & O_EXCL && didExist) return std::unexpected(EEXIST);
        node = node->Reduce(followSymlinks, false);
        if (!node) return std::unexpected(ENOENT);

        if (node->IsSymlink()) return std::unexpected(ELOOP);
        if ((flags & O_DIRECTORY && !node->IsDirectory()))
            return std::unexpected(ENOTDIR);
        if ((node->IsDirectory() && (acc & O_WRONLY || acc & O_RDWR)))
            return std::unexpected(EISDIR);

        // TODO(v1tr10l7): check acc modes and truncate
        if (flags & O_TRUNC && node->IsRegular() && didExist)
            ;
        return new FileDescriptor(node, flags, accMode);
    }

    ErrorOr<INode*> ResolvePath(PathView path)
    {
        INode* node = std::get<1>(ResolvePath(s_RootNode, path));
        if (!node) return std::unexpected(errno);

        return node;
    }
    std::tuple<INode*, INode*, std::string>
    ResolvePath(INode* parent, std::string_view path, bool automount)
    {
        if (!parent || Path::IsAbsolute(path)) parent = GetRootNode();

        auto currentNode
            = parent == s_RootNode ? s_RootNode : parent->Reduce(false);

        if (path == "/" || path.empty()) return {currentNode, currentNode, "/"};

        auto getParent = [&currentNode]
        {
            if (currentNode == GetRootNode()->Reduce(false)) return currentNode;
            else if (currentNode == currentNode->GetFilesystem()->GetRootNode())
                return currentNode->GetFilesystem()
                    ->GetMountedOn()
                    ->GetParent();

            return currentNode->GetParent();
        };

        auto segments = Path::SplitPath(path.data());

        for (usize i = 0; i < segments.size(); i++)
        {
            auto segment     = segments[i];
            bool isLast      = i == (segments.size() - 1);

            bool previousDir = segment == "..";
            bool currentDir  = segment == ".";

            if (currentDir || previousDir)
            {
                if (previousDir) currentNode = getParent();

                if (isLast)
                    return {getParent(), currentNode, currentNode->GetName()};

                continue;
            }

            if (currentNode->GetChildren().contains(segment)
                || currentNode->GetFilesystem()->Populate(currentNode))
            {
                auto node = currentNode->GetChildren()[segment]->Reduce(
                    true, isLast ? automount : true);

                auto getReal = [](INode* node) -> INode*
                {
                    if (node->IsFilesystemRoot())
                        return node->GetFilesystem()->GetMountedOn();
                    return node;
                };

                if (isLast)
                    return {currentNode, getReal(node), node->GetName()};

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
                return {
                    currentNode, nullptr, {segment.data(), segment.length()}};

            break;
        }

        errno = ENOENT;
        return {nullptr, nullptr, ""};
    }

    std::unordered_map<std::string_view, class Filesystem*>& GetMountPoints()
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
    bool Mount(INode* parent, PathView source, PathView target,
               std::string_view fsName, i32 flags, void* data)
    {
        ScopedLock  guard(s_Lock);

        Filesystem* fs = CreateFilesystem(fsName, flags);
        if (!fs)
        {
            errno = ENODEV;
            return false;
        }
        [[maybe_unused]] INode* sourceNode = nullptr;

        auto [nparent, node, basename]     = ResolvePath(parent, target);
        bool   isRoot                      = (node == GetRootNode());
        INode* mountGate                   = nullptr;

        if (!node) goto fail;

        if (!isRoot && !node->IsDirectory()) return_err(false, ENOTDIR);

        mountGate = fs->Mount(parent, nullptr, node, basename, data);
        if (!mountGate) goto fail;

        node->mountGate = mountGate;

        if (source.GetSize() == 0)
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName.data(),
                     target.Raw());
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     source.Raw(), target.Raw(), fsName.data());

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

        newNode = newNodeParent->GetFilesystem()->CreateNode(newNodeParent,
                                                             newNodeName, mode);
        if (newNode) newNodeParent->InsertChild(newNode, newNode->GetName());
        return newNode;
    }
    ErrorOr<const stat*> Stat(i32 fdNum, std::string_view path, i32 flags)
    {
        if (flags & ~(AT_EMPTY_PATH | AT_NO_AUTOMOUNT | AT_SYMLINK_NOFOLLOW))
            return std::unexpected(Error(EINVAL));

        Process*        process        = Process::GetCurrent();
        FileDescriptor* fd             = process->GetFileHandle(fdNum);
        bool            followSymlinks = !(flags & AT_SYMLINK_NOFOLLOW);

        if (!path.data() || !path[0])
        {
            if (!(flags & AT_EMPTY_PATH)) return std::unexpected(ENOENT);

            if (fdNum == AT_FDCWD) return &process->GetCWD()->GetStats();
            else if (!fd) return std::unexpected(EBADF);

            return &fd->GetNode()->GetStats();
        }

        INode* parent = Path::IsAbsolute(path) ? VFS::GetRootNode() : nullptr;
        if (fdNum == AT_FDCWD) parent = process->GetCWD();
        else if (fd) parent = fd->GetNode();

        if (!Path::ValidateLength(path)) return std::unexpected(ENAMETOOLONG);
        if (!parent) return std::unexpected(EBADF);
        INode* node
            = std::get<1>(VFS::ResolvePath(parent, path, followSymlinks));

        if (!node) return std::unexpected(errno);
        return &node->GetStats();
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
