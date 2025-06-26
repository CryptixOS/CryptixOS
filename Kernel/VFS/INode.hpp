/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Library/UserBuffer.hpp>

#include <Prism/Delegate.hpp>
#include <Prism/Memory/Ref.hpp>

#include <VFS/Filesystem.hpp>
#include <VFS/VFS.hpp>

#include <errno.h>

class DirectoryEntry;
class FileDescriptor;

struct Credentials;
class INode
{
  public:
    struct Attributes
    {
        mode_t   Mode             = 0;
        uid_t    UID              = 0;
        gid_t    GID              = 0;
        loff_t   Size             = 0;
        timespec AccessTime       = {};
        timespec ModificationTime = {};
        timespec ChangeTime       = {};
    };

    INode(StringView name);
    INode(StringView name, class Filesystem* fs);
    virtual ~INode() {}

    // virtual Ref<DirectoryEntry> Ref() { return nullptr; }

    StringView               GetTarget() const { return m_Target; }

    inline class Filesystem* Filesystem() { return m_Filesystem; }
    virtual const stat&      Stats() { return m_Stats; }

    using DirectoryIterator
        = Delegate<bool(StringView name, loff_t offset, usize ino, u64 type)>;
    virtual ErrorOr<void> TraverseDirectories(class DirectoryEntry* parent,
                                              DirectoryIterator     iterator)
    {
        return Error(ENOSYS);
    }

    virtual INode* Lookup(const String& name) { return nullptr; }
    virtual ErrorOr<DirectoryEntry*> Lookup(DirectoryEntry* entry);

    inline StringView                Name() { return m_Name; }
    mode_t                           Mode() const;

    bool                             IsFilesystemRoot() const;
    bool                             IsEmpty();
    bool                             CanWrite(const Credentials& creds) const;

    inline bool   IsCharDevice() const { return S_ISCHR(m_Stats.st_mode); }
    inline bool   IsFifo() const { return S_ISFIFO(m_Stats.st_mode); }
    inline bool   IsDirectory() const { return S_ISDIR(m_Stats.st_mode); }
    inline bool   IsRegular() const { return S_ISREG(m_Stats.st_mode); }
    inline bool   IsSymlink() const { return S_ISLNK(m_Stats.st_mode); }
    inline bool   IsSocket() const { return S_ISSOCK(m_Stats.st_mode); }

    bool          ValidatePermissions(const Credentials& creds, u32 acc);
    void          UpdateATime();

    virtual void  InsertChild(INode* node, StringView name)            = 0;
    virtual isize Read(void* buffer, off_t offset, usize bytes)        = 0;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) = 0;
    virtual i32   IoCtl(usize request, usize arg) { return_err(-1, ENODEV); }
    virtual ErrorOr<isize> Truncate(usize size) { return Error(ENOSYS); }
    virtual ErrorOr<void>  Rename(INode* newParent, StringView newName)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> MkDir(StringView name, mode_t mode, uid_t uid = 0,
                                gid_t gid = 0)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void>  Link(PathView path) { return Error(ENOSYS); }
    virtual ErrorOr<isize> ReadLink(UserBuffer& outBuffer);

    virtual ErrorOr<isize> CheckPermissions(mode_t mask)
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<void> UpdateTimestamps(timespec atime = {},
                                           timespec mtime = {},
                                           timespec ctime = {});
    virtual ErrorOr<void> ChMod(mode_t mode) { return Error(ENOSYS); }
    virtual ErrorOr<void> FlushMetadata() { return Error(ENOSYS); }

    inline bool           Populate()
    {
        return true;
        // return (!m_Populated && IsDirectory()) ? m_Filesystem->Populate()
        //                                        : true;
    }

  protected:
    INode*            m_Parent;
    String            m_Name;
    String            m_Target;
    Spinlock          m_Lock;
    Attributes        m_Attributes = {};

    class Filesystem* m_Filesystem;
    stat              m_Stats;
    bool              m_Populated = false;
    bool              m_Dirty     = false;
};

using INodeAttributes = INode::Attributes;
