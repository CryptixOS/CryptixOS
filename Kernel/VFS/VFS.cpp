
/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Spinlock.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/INode.hpp>
#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/VFS.hpp>

#include <cerrno>

namespace VFS
{
    static INode*   s_RootNode = nullptr;
    static Spinlock s_Lock;

    INode*          GetRootNode()
    {
        Thread* thread = CPU::GetCurrentThread();
        if (!thread) return s_RootNode;

        Process* process = thread->parent;
        Assert(process);

        INode* rootNode = process->GetRootNode();
        return rootNode ? rootNode : s_RootNode;
    }

    static Filesystem* CreateFilesystem(std::string_view name, u32 flags)
    {
        Filesystem* fs = nullptr;
        if (name == "tmpfs") fs = new TmpFs(flags);
        else if (name == "devtmpfs") fs = new DevTmpFs(flags);

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

    FileDescriptor* Open(INode* parent, PathView path, i32 flags, mode_t mode)
    {
        Process* current        = CPU::GetCurrentThread()->parent;
        bool     followSymlinks = !(flags & O_NOFOLLOW);
        auto     acc            = flags & O_ACCMODE;

        INode*   node

            = std::get<1>(VFS::ResolvePath(parent, path, followSymlinks));
        bool didExist = true;

        if (!node)
        {
            didExist = false;
            if (errno != ENOENT || !(flags & O_CREAT)) return nullptr;

            if (!parent->ValidatePermissions(current->GetCredentials(), 5))
                return_err(nullptr, EACCES);
            node = VFS::CreateNode(parent, path,
                                   (mode & ~current->GetUMask()) | S_IFREG);

            if (!node) return nullptr;
        }

        if (flags & O_EXCL && didExist) return_err(nullptr, EEXIST);
        node = node->Reduce(followSymlinks, false);
        if (!node) return nullptr;

        if (node->IsSymlink()) return_err(nullptr, ELOOP);
        if ((flags & O_DIRECTORY && !node->IsDirectory())
            || (node->IsDirectory() && (acc & O_WRONLY || acc & O_RDWR)))
            return_err(nullptr, EISDIR);

        // TODO(v1tr10l7): check acc modes and truncate
        if (flags & O_TRUNC && node->IsRegular() && didExist)
            ;
        return node->Open(flags, mode);
    }

    std::tuple<INode*, INode*, std::string>
    ResolvePath(INode* parent, std::string_view path, bool automount)
    {
        if (!parent || Path::IsAbsolute(path)) parent = GetRootNode();

        auto currentNode = parent->Reduce(false);

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

        return s_RootNode != nullptr;
    }

    // TODO: flags
    bool Mount(INode* parent, PathView source, PathView target,
               std::string_view fsName, i32 flags, void* data)
    {
        ScopedLock  guard(s_Lock);

        Filesystem* fs = CreateFilesystem(fsName, flags);
        if (fs == nullptr)
        {
            errno = ENODEV;
            return false;
        }
        [[maybe_unused]] INode* sourceNode = nullptr;

        auto [nparent, node, basename]     = ResolvePath(parent, target);
        bool isRoot                        = (node == GetRootNode());

        if (!node) return false;

        if (!isRoot && !node->IsDirectory()) return_err(false, ENOTDIR);

        auto mountGate = fs->Mount(parent, nullptr, node, basename, data);
        if (!mountGate) return false;

        node->mountGate = mountGate;

        if (source.length() == 0)
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName.data(),
                     target.data());
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     source.data(), target.data(), fsName.data());
        return true;
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
