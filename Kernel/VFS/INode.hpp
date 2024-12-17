/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "API/UnixTypes.hpp"

#include "VFS/Filesystem.hpp"
#include "VFS/VFS.hpp"

#include <functional>
#include <unordered_map>

enum class INodeType
{
    eRegular         = 0,
    eHardLink        = 1,
    eSymlink         = 2,
    eCharacterDevice = 3,
    eBlockDevice     = 4,
    eDirectory       = 5,
};

struct FileDescriptor;

class INode
{
  public:
    std::string target;
    INode*      mountGate;

    INode(INode* parent, std::string_view name, Filesystem* fs)
        : parent(parent)
        , name(name)
        , filesystem(fs)
    {
    }
    INode(std::string_view name, INodeType type)
        : name(name)
        , type(type)
    {
    }
    INode(INode* parent, std::string_view name, Filesystem* fs, INodeType type)
        : parent(parent)
        , name(name)
        , filesystem(fs)
        , type(type)
    {
    }
    virtual ~INode() {}

    INode*             Reduce(bool symlinks, bool automount = true);
    std::string        GetPath();

    inline Filesystem* GetFilesystem() { return filesystem; }
    inline const stat& GetStats() const { return stats; }
    inline std::unordered_map<std::string_view, INode*>& GetChildren()
    {
        return children;
    }
    inline const std::string& GetName() { return name; }
    inline INode*             GetParent() { return parent; }
    mode_t                    GetMode() const;
    INodeType                 GetType() const { return type; }

    bool                      IsEmpty();

    FileDescriptor*           Open();
    virtual void  InsertChild(INode* node, std::string_view name)      = 0;
    virtual isize Read(void* buffer, off_t offset, usize bytes)        = 0;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) = 0;
    virtual i32   IoCtl(usize request, usize arg) { return_err(-1, ENOTTY); }

  protected:
    INode*                                       parent;
    std::string                                  name;
    std::mutex                                   lock;
    Filesystem*                                  filesystem;
    stat                                         stats;
    std::unordered_map<std::string_view, INode*> children;

    INodeType                                    type;

    INode* InternalReduce(bool symlinks, bool automount, usize cnt);
};
