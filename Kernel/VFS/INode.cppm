/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:VFS.INode;

export import :Library.Locking.Spinlock;
export import :Library.UserBuffer;

// ---- merged from VFS/DirectoryEntry.hpp (the two types refer to each other) ----

export class INode;

export enum class DirectoryEntryFlags
{
    eNegative      = 0,
    eDirectory     = 2,
    eRegular       = 4,
    eSpecial       = 5,
    eSymlink       = 6,
    eEntryTypeMask = 7,

    eMountPoint    = Bit(5),
};

export constexpr DirectoryEntryFlags& operator|=(DirectoryEntryFlags&      lhs,
                                          const DirectoryEntryFlags rhs)
{
    return lhs = static_cast<DirectoryEntryFlags>(ToUnderlying(lhs)
                                                  | ToUnderlying(rhs));
}
export constexpr DirectoryEntryFlags& operator&=(DirectoryEntryFlags&      lhs,
                                          const DirectoryEntryFlags rhs)
{
    return lhs = static_cast<DirectoryEntryFlags>(ToUnderlying(lhs)
                                                  & ToUnderlying(rhs));
}
export constexpr DirectoryEntryFlags operator~(const DirectoryEntryFlags flags)
{
    return static_cast<DirectoryEntryFlags>(~ToUnderlying(flags));
}
export constexpr bool operator&(const DirectoryEntryFlags lhs,
                         const DirectoryEntryFlags rhs)
{
    return ToUnderlying(lhs) & ToUnderlying(rhs);
}

export class DirectoryEntry : public RefCounted
{
  public:
    explicit DirectoryEntry(StringView name);
    DirectoryEntry(::WeakRef<DirectoryEntry> parent, StringView name);
    DirectoryEntry(const DirectoryEntry& other);

    virtual ~DirectoryEntry();

    ::Ref<class INode>               INode() const;
    inline StringView                Name() const { return m_Name; }
    inline ::WeakRef<DirectoryEntry> Parent() { return m_Parent; }

    Path                             Path();
    const UnorderedMap<String, ::Ref<DirectoryEntry>>& Children() const;

    void                      SetParent(::WeakRef<DirectoryEntry> entry);
    void                      SetMountGate(::Ref<DirectoryEntry> mountGate);
    void                      Bind(::Ref<class INode> inode);
    void                      InsertChild(::Ref<class DirectoryEntry> entry);
    void                      RemoveChild(::Ref<class DirectoryEntry> entry);

    ::WeakRef<DirectoryEntry> FollowSymlinks(usize cnt = 0);

    WeakRef<DirectoryEntry>   GetEffectiveParent();

    using Iterator
        = UnorderedMap<String, ::Ref<class DirectoryEntry>>::Iterator<>;
    using ConstIterator
        = UnorderedMap<String, ::Ref<class DirectoryEntry>>::ConstIterator;
    Iterator      begin() { return m_Children.begin(); }
    ConstIterator begin() const { return m_Children.begin(); }
    Iterator      end() { return m_Children.end(); }
    ConstIterator end() const { return m_Children.end(); }

    using DirectoryIterator
        = Delegate<bool(StringView name, loff_t offset, usize ino, u64 type)>;
    ErrorOr<void> TraverseDirectories(::Ref<class DirectoryEntry> parent,
                                      DirectoryIterator           iterator);
    ::Ref<DirectoryEntry> Lookup(const String& name);
    ErrorOr<void>         PopulateDirectoryEntries();

    bool                  IsMountPoint() const;
    bool                  IsDirectory() const;
    bool                  IsRegular() const;
    bool                  IsSymlink() const;
    bool                  IsSpecial() const;

  private:
    friend class INode;

    Spinlock                  m_Lock;

    String                    m_Name      = ""_s;
    DirectoryEntryFlags       m_Flags     = DirectoryEntryFlags::eNegative;
    ::Ref<class INode>        m_INode     = nullptr;
    bool                      m_Populated = false;
    usize                     m_DirOffset = 0;

    ::WeakRef<DirectoryEntry> m_Parent    = nullptr;
    UnorderedMap<String, ::Ref<class DirectoryEntry>> m_Children;
};

export class FileDescriptor;

export using INodeID = ino_t;

export struct Credentials;
export class File;
export class INode : public RefCounted
{
  public:
    struct Metadata
    {
        INodeID   ID           = -1;
        INodeMode Mode         = 0;
        usize     Size         = 0;
        usize     LinkCount    = 0;

        usize     BlockSize    = 0;
        usize     BlockCount   = 0;

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
    virtual const stat       Stats() const;

    using DirectoryIterator
        = Delegate<bool(StringView name, loff_t offset, usize ino, u64 type)>;
    virtual ErrorOr<void>
    TraverseDirectories(::Ref<class DirectoryEntry> parent,
                        DirectoryIterator           iterator)
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<::Ref<DirectoryEntry>> Lookup(::Ref<DirectoryEntry> dentry);

    inline StringView                      Name() const { return m_Name; }
    DeviceID                               BackingDeviceID() const;
    INodeID                                ID() const;
    INodeMode                              Mode() const;
    nlink_t                                LinkCount() const;
    ::UserID                               UserID() const;
    ::GroupID                              GroupID() const;
    DeviceID                               DeviceID() const;
    isize                                  Size() const;
    blksize_t                              BlockSize() const;
    blkcnt_t                               BlockCount() const;

    timespec                               AccessTime() const;
    timespec                               ModificationTime() const;
    timespec                               StatusChangeTime() const;

    bool                                   IsFilesystemRoot() const;
    bool                                   IsEmpty();
    bool                                   ReadOnly();
    bool                                   Immutable();
    bool        CanRead(const Credentials& creds) const;
    bool        CanWrite(const Credentials& creds) const;
    bool        CanExecute(const Credentials& creds) const;

    inline bool IsCharDevice() const { return S_ISCHR(m_Metadata.Mode); }
    inline bool IsBlockDevice() const { return S_ISBLK(m_Metadata.Mode); }
    inline bool IsFifo() const { return S_ISFIFO(m_Metadata.Mode); }
    inline bool IsDirectory() const { return S_ISDIR(m_Metadata.Mode); }
    inline bool IsRegular() const { return S_ISREG(m_Metadata.Mode); }
    inline bool IsSymlink() const { return S_ISLNK(m_Metadata.Mode); }
    inline bool IsSocket() const { return S_ISSOCK(m_Metadata.Mode); }

    inline bool IsSetUserID() const { return m_Metadata.Mode & S_ISUID; }
    inline bool IsSetGroupID() const { return m_Metadata.Mode & S_ISGID; }

    bool        ValidatePermissions(const Credentials& creds, u32 acc);
    void        UpdateATime();

    virtual ErrorOr<File*> Open(class ::Ref<::DirectoryEntry> dentry, i64 flags,
                                u64 accMode);
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode, dev_t dev = 0);
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateFile(::Ref<DirectoryEntry> entry, INodeMode mode);
    virtual ErrorOr<::Ref<DirectoryEntry>>
    CreateDirectory(::Ref<DirectoryEntry> entry, INodeMode mode);
    virtual ErrorOr<::Ref<DirectoryEntry>> Symlink(::Ref<DirectoryEntry> entry,
                                                   PathView targetPath);
    virtual ErrorOr<::Ref<DirectoryEntry>> Link(::Ref<DirectoryEntry> oldEntry,
                                                ::Ref<DirectoryEntry> entry);

    virtual void  InsertChild(::Ref<INode> node, StringView name)      = 0;
    virtual isize Read(void* buffer, off_t offset, usize bytes)        = 0;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) = 0;
    virtual ErrorOr<isize> IoCtl(usize request, usize arg)
    {
        return Error(ENODEV);
    }
    virtual ErrorOr<Path>  ReadLink();

    virtual ErrorOr<isize> Truncate(usize size) { return Error(ENOSYS); }
    virtual ErrorOr<void>  Rename(::Ref<INode> newParent, StringView newName)
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<void> Unlink(::Ref<DirectoryEntry> entry);
    virtual ErrorOr<void> RmDir(::Ref<DirectoryEntry> entry)
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<isize> CheckPermissions(INodeMode mask);

    virtual ErrorOr<void>  SetOwner(::UserID uid, ::GroupID gid);
    virtual ErrorOr<void>  ChangeMode(INodeMode mode);
    virtual ErrorOr<void>  UpdateTimestamps(timespec atime = {},
                                            timespec mtime = {},
                                            timespec ctime = {});

    virtual ErrorOr<isize> SetExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                                StringView            name,
                                                const u8* value, usize size,
                                                isize flags)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> GetExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                                StringView name, u8* value,
                                                usize size)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> ListExtendedAttributes(::Ref<DirectoryEntry> dentry,
                                                  char* list, usize size)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> RemoveExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                                   StringView            name)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> FlushMetadata() { return Error(ENOSYS); }

  protected:
    INode*            m_Parent;
    String            m_Name;
    Spinlock          m_Lock;
    Metadata          m_Metadata = {};

    class Filesystem* m_Filesystem;
    bool              m_Populated = false;
    bool              m_Dirty     = false;
};

export using INodeMetadata = INode::Metadata;
