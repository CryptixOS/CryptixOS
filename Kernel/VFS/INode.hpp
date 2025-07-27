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

#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>

#include <VFS/DirectoryEntry.hpp>

#include <errno.h>

class FileDescriptor;

using INodeID   = ino_t;
using INodeMode = mode_t;
using DeviceID  = dev_t;
using UserID    = uid_t;
using GroupID   = gid_t;

struct Credentials;
class INode
{
  public:
    struct Metadata
    {
        INodeID   ID           = -1;
        INodeMode Mode         = 0;
        usize     Size         = 0;
        usize     LinkCount    = 0;

        usize     BlockCount   = 0;
        usize     BlockSize    = 0;

        DeviceID  RootDeviceID = 0;
        DeviceID  DeviceID     = 0;

        ::UserID  UID          = 0;
        ::GroupID GID          = 0;

        timespec  AccessTime{};
        timespec  ModificationTime{};
        timespec  ChangeTime{};
    };

    INode(class Filesystem* fs);
    INode(StringView name);
    INode(StringView name, class Filesystem* fs);
    virtual ~INode() {}

    inline class Filesystem* Filesystem() { return m_Filesystem; }
    virtual const stat       Stats();

    using DirectoryIterator
        = Delegate<bool(StringView name, loff_t offset, usize ino, u64 type)>;
    virtual ErrorOr<void> TraverseDirectories(Ref<class DirectoryEntry> parent,
                                              DirectoryIterator iterator)
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<Ref<DirectoryEntry>> Lookup(Ref<DirectoryEntry> dentry);

    inline StringView                    Name() { return m_Name; }
    DeviceID                             BackingDeviceID() const;
    INodeID                              ID() const;
    INodeMode                            Mode() const;
    nlink_t                              LinkCount() const;
    ::UserID                             UserID() const;
    ::GroupID                            GroupID() const;
    DeviceID                             DeviceID() const;
    isize                                Size() const;
    blksize_t                            BlockSize() const;
    blkcnt_t                             BlockCount() const;

    timespec                             AccessTime() const;
    timespec                             ModificationTime() const;
    timespec                             StatusChangeTime() const;

    bool                                 IsFilesystemRoot() const;
    bool                                 IsEmpty();
    bool                                 ReadOnly();
    bool                                 Immutable();
    bool        CanWrite(const Credentials& creds) const;

    inline bool IsCharDevice() const { return S_ISCHR(m_Metadata.Mode); }
    inline bool IsFifo() const { return S_ISFIFO(m_Metadata.Mode); }
    inline bool IsDirectory() const { return S_ISDIR(m_Metadata.Mode); }
    inline bool IsRegular() const { return S_ISREG(m_Metadata.Mode); }
    inline bool IsSymlink() const { return S_ISLNK(m_Metadata.Mode); }
    inline bool IsSocket() const { return S_ISSOCK(m_Metadata.Mode); }

    bool        ValidatePermissions(const Credentials& creds, u32 acc);
    void        UpdateATime();

    virtual ErrorOr<Ref<DirectoryEntry>> CreateNode(Ref<DirectoryEntry> entry,
                                                    mode_t mode, dev_t dev = 0);
    virtual ErrorOr<Ref<DirectoryEntry>> CreateFile(Ref<DirectoryEntry> entry,
                                                    mode_t              mode);
    virtual ErrorOr<Ref<DirectoryEntry>>
    CreateDirectory(Ref<DirectoryEntry> entry, mode_t mode);
    virtual ErrorOr<Ref<DirectoryEntry>> Symlink(Ref<DirectoryEntry> entry,
                                                 PathView targetPath);
    virtual ErrorOr<Ref<DirectoryEntry>> Link(Ref<DirectoryEntry> oldEntry,
                                              Ref<DirectoryEntry> entry);

    virtual void  InsertChild(INode* node, StringView name)            = 0;
    virtual isize Read(void* buffer, off_t offset, usize bytes)        = 0;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) = 0;
    virtual ErrorOr<isize> IoCtl(usize request, usize arg)
    {
        return Error(ENODEV);
    }
    virtual ErrorOr<Path>  ReadLink();

    virtual ErrorOr<isize> Truncate(usize size) { return Error(ENOSYS); }
    virtual ErrorOr<void>  Rename(INode* newParent, StringView newName)
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<void> Unlink(Ref<DirectoryEntry> entry);
    virtual ErrorOr<void> RmDir(Ref<DirectoryEntry> entry)
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<isize> CheckPermissions(INodeMode mask);

    virtual ErrorOr<void>  SetOwner(::UserID uid, ::GroupID gid);
    virtual ErrorOr<void>  ChangeMode(INodeMode mode);
    virtual ErrorOr<void>  UpdateTimestamps(timespec atime = {},
                                            timespec mtime = {},
                                            timespec ctime = {});
    virtual ErrorOr<void>  FlushMetadata() { return Error(ENOSYS); }

  protected:
    INode*            m_Parent;
    String            m_Name;
    Spinlock          m_Lock;
    Metadata          m_Metadata = {};

    class Filesystem* m_Filesystem;
    bool              m_Populated = false;
    bool              m_Dirty     = false;
};

using INodeMetadata = INode::Metadata;
