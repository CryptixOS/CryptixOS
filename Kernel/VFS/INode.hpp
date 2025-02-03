/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

#include <Utility/Spinlock.hpp>

#include <VFS/Filesystem.hpp>
#include <VFS/VFS.hpp>

#include <errno.h>
#include <unordered_map>

class FileDescriptor;

struct Credentials;
class INode
{
  public:
    std::string target;
    INode*      mountGate;

    INode(std::string_view name)
        : m_Name(name)
    {
    }
    INode(INode* parent, std::string_view name, Filesystem* fs);
    virtual ~INode() {}

    INode*      Reduce(bool symlinks, bool automount = true, usize cnt = 0);
    std::string GetPath();
    std::string_view    GetTarget() const { return target; }

    inline Filesystem*  GetFilesystem() { return m_Filesystem; }
    virtual const stat& GetStats() { return m_Stats; }
    virtual std::unordered_map<std::string_view, INode*>& GetChildren()
    {
        return m_Children;
    }
    inline const std::string& GetName() { return m_Name; }
    inline INode*             GetParent() { return m_Parent; }
    mode_t                    GetMode() const;

    inline bool               IsFilesystemRoot() const
    {
        return m_Filesystem->GetMountedOn()
            && this == m_Filesystem->GetMountedOn()->mountGate;
    }
    bool          IsEmpty();
    bool          CanWrite(const Credentials& creds) const;

    inline bool   IsCharDevice() const { return S_ISCHR(m_Stats.st_mode); }
    inline bool   IsFifo() const { return S_ISFIFO(m_Stats.st_mode); }
    inline bool   IsDirectory() const { return S_ISDIR(m_Stats.st_mode); }
    inline bool   IsRegular() const { return S_ISREG(m_Stats.st_mode); }
    inline bool   IsSymlink() const { return S_ISLNK(m_Stats.st_mode); }
    inline bool   IsSocket() const { return S_ISSOCK(m_Stats.st_mode); }

    bool          ValidatePermissions(const Credentials& creds, u32 acc);
    void          UpdateATime();

    virtual void  InsertChild(INode* node, std::string_view name)      = 0;
    virtual isize Read(void* buffer, off_t offset, usize bytes)        = 0;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) = 0;
    virtual i32   IoCtl(usize request, usize arg) { return_err(-1, ENODEV); }
    virtual ErrorOr<isize> Truncate(usize size) = 0;

  protected:
    INode*                                       m_Parent;
    std::string                                  m_Name;
    Spinlock                                     m_Lock;

    Filesystem*                                  m_Filesystem;
    std::unordered_map<std::string_view, INode*> m_Children;
    stat                                         m_Stats;
};
