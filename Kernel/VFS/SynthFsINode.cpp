/*
 * Created by v1tr10l7 on 28.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Scheduler/Process.hpp>
#include <Time/Time.hpp>

#include <VFS/Filesystem.hpp>
#include <VFS/SynthFsINode.hpp>

SynthFsINode::SynthFsINode(StringView name, class Filesystem* fs, INodeID id,
                           INodeMode mode)
    : INode(name, fs)
{
    m_Metadata.ID           = id;
    m_Metadata.Mode         = mode;
    // Every directory has at least two entries: '.' and '..'
    m_Metadata.Size         = IsDirectory() ?: DIRECTORY_ENTRY_SIZE * 2;
    m_Metadata.LinkCount    = 1 + IsDirectory();

    m_Metadata.BlockSize    = PMM::PAGE_SIZE;
    m_Metadata.BlockCount   = 0;

    m_Metadata.RootDeviceID = fs->BackingDeviceID();
    m_Metadata.DeviceID     = 0;

    auto process            = Process::Current();
    if (process)
    {
        const auto& creds = process->Credentials();
        m_Metadata.GID    = creds.GroupID;
        m_Metadata.UID    = creds.UserID;
    }

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
}

ErrorOr<void>
SynthFsINode::TraverseDirectories(::Ref<class DirectoryEntry> parent,
                                  DirectoryIterator           iterator)
{
    ScopedLock guard(m_Lock);
    usize      offset = 0;
    for (const auto& [name, inode] : Children())
    {
        usize     ino  = inode->ID();
        INodeMode mode = inode->Mode();
        auto      type = IF2DT(mode);

        if (!iterator(name, offset, ino, type)) break;
        ++offset;
    }

    return {};
}
ErrorOr<::Ref<DirectoryEntry>> SynthFsINode::Lookup(::Ref<DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);

    auto       child = Children().Find(entry->Name());
    if (child != Children().end())
    {
        entry->Bind(child->Value);
        return entry;
    }

    return Error(ENOENT);
}

ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode, dev_t dev)
{
    ScopedLock guard(m_Lock);
    if (m_Children.Contains(entry->Name())) return Error(EEXIST);
    auto inode = TryOrRet(m_Filesystem->AllocateNode(entry->Name(), mode))
                     .As<SynthFsINode>();

    inode->m_Parent = this;
    if (inode->IsRegular())
    {
        // A new file is empty; the storage is reserved up front
        inode->m_Metadata.Size = 0;
        inode->m_Buffer.Reserve(GetDefaultSize());
    }

    inode->m_Metadata.DeviceID = dev;
    m_Metadata.Size += DIRECTORY_ENTRY_SIZE;

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};
    UpdateTimestamps(atime, currentTime, currentTime);

    m_Lock.Release();
    InsertChild(inode, entry->Name());
    m_Lock.Acquire();

    if (Mode() & S_ISGID)
    {
        inode->m_Metadata.GID = m_Metadata.GID;
        if (inode->IsDirectory()) inode->m_Metadata.Mode |= S_ISGID;
    }

    entry->Bind(inode);
    return entry;
}
ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::CreateFile(::Ref<DirectoryEntry> entry, INodeMode mode)
{
    return CreateNode(entry, (mode & ~S_IFMT) | S_IFREG, 0);
}
ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::CreateDirectory(::Ref<DirectoryEntry> entry, INodeMode mode)
{

    IgnoreUnused(TryOrRet(CreateNode(entry, (mode & ~S_IFMT) | S_IFDIR, 0)));

    ++m_Metadata.LinkCount;
    return entry;
}
ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::Symlink(::Ref<DirectoryEntry> entry, PathView targetPath)
{
    auto dentry     = TryOrRet(CreateNode(entry, 0777 | S_IFLNK, 0));
    auto inode      = dentry->INode().As<SynthFsINode>();

    inode->m_Target = targetPath;
    return dentry;
}
ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::Link(::Ref<DirectoryEntry> oldEntry, ::Ref<DirectoryEntry> entry)
{
    auto inode = oldEntry->INode().As<SynthFsINode>();
    if (inode->IsDirectory()) return Error(EPERM);

    m_Metadata.Size += DIRECTORY_ENTRY_SIZE;

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};
    UpdateTimestamps(atime, currentTime, currentTime);

    inode->m_Metadata.ChangeTime = currentTime;
    ++inode->m_Metadata.LinkCount;

    entry->Bind(inode);
    return entry;
}

void SynthFsINode::InsertChild(::Ref<INode> inode, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = inode;
}

isize SynthFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    Assert(buffer);
    Assert(offset >= 0);

    ScopedLock guard(m_Lock);
    bytes = Min(bytes, m_Buffer.Size() - offset);

    if (m_Filesystem->ShouldUpdateATime()) UpdateTimestamps(Time::GetReal());
    m_Buffer.Read(offset, reinterpret_cast<Byte*>(buffer), bytes);

    return bytes;
}
isize SynthFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    Assert(buffer);
    Assert(offset >= 0);

    ScopedLock guard(m_Lock);

    // The file ends where the data written does; only the storage behind it
    // grows in powers of two, so appending stays amortized O(1)
    usize end = offset + bytes;
    if (end > m_Buffer.Capacity())
    {
        usize newCapacity = m_Buffer.Capacity() ?: PMM::PAGE_SIZE;
        while (end > newCapacity) newCapacity *= 2;
        m_Buffer.Reserve(newCapacity);
    }
    if (end > m_Buffer.Size()) ResizeBuffer(end);

    // TODO(v1tr10l7): Handle filesystem memory limits
    m_Buffer.Write(offset, reinterpret_cast<const Byte*>(buffer), bytes);
    auto currentTime = Time::GetReal();
    if (m_Filesystem->ShouldUpdateATime())
        UpdateTimestamps(currentTime, currentTime);

    return bytes;
}
ErrorOr<Path>  SynthFsINode::ReadLink() { return m_Target; }

ErrorOr<isize> SynthFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);
    if (size == m_Buffer.Size()) return 0;

    RetOnError(ResizeBuffer(size));
    m_Buffer.ShrinkToFit();

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};
    UpdateTimestamps(atime, currentTime, currentTime);

    return 0;
}

ErrorOr<void> SynthFsINode::Rename(::Ref<INode> newParent, StringView newName)
{
    // TODO(v1tr10l7): Remove old inode

    // auto parent = reinterpret_cast<TmpFsINode*>(m_Parent);
    // parent->m_Children.Erase(m_Name);

    m_Name = newName;
    newParent->InsertChild(this, Name());

    return {};
}

ErrorOr<void> SynthFsINode::Unlink(::Ref<DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);
    auto       it = m_Children.Find(entry->Name());
    if (it == m_Children.end()) return {};

    auto inode = entry->INode().As<SynthFsINode>();
    m_Metadata.Size -= DIRECTORY_ENTRY_SIZE;

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};

    UpdateTimestamps(atime, currentTime, currentTime);
    inode->m_Metadata.ChangeTime = currentTime;
    --inode->m_Metadata.LinkCount;

    auto inodeLinkCount = inode->m_Metadata.LinkCount;
    if (inodeLinkCount == 0 + inode->IsDirectory())
    {
        m_Children.Erase(it);
        IgnoreUnused(m_Filesystem->FreeINode(inode));
    }

    auto parent = entry->Parent();
    Assert(parent);
    parent->RemoveChild(entry);

    return {};
}
ErrorOr<void> SynthFsINode::RmDir(::Ref<DirectoryEntry> entry)
{
    auto inode = entry->INode().As<SynthFsINode>();
    if (!inode->Children().Empty()) return Error(ENOTEMPTY);

    m_Lock.Acquire();
    --m_Metadata.LinkCount;
    m_Lock.Release();

    return Unlink(entry);
}

namespace
{
    // setxattr(2) flags
    constexpr isize XATTR_CREATE  = 1;
    constexpr isize XATTR_REPLACE = 2;
} // namespace

ErrorOr<isize> SynthFsINode::SetExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                                  StringView            name,
                                                  const u8* value, usize size,
                                                  isize flags)
{
    if (flags & ~(XATTR_CREATE | XATTR_REPLACE)) return Error(EINVAL);
    if (size > System::Limits::XATTR_MAX_VALUE_SIZE) return Error(E2BIG);

    // Copy the value in before touching the map, so a bad pointer leaves the
    // attribute as it was
    Vector<u8> data(size);
    if (size && !CopyFromUser(data.Raw(), value, size)) return Error(EFAULT);

    ScopedLock guard(m_Lock);
    bool       exists
        = m_ExtendedAttributes.Find(String(name)) != m_ExtendedAttributes.end();
    if ((flags & XATTR_CREATE) && exists) return Error(EEXIST);
    if ((flags & XATTR_REPLACE) && !exists) return Error(ENODATA);

    m_ExtendedAttributes[String(name)] = Move(data);
    return 0;
}
ErrorOr<isize> SynthFsINode::GetExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                                  StringView name, u8* value,
                                                  usize size)
{
    // Copy the value out under the lock, and to user memory without it
    Vector<u8> data;
    {
        ScopedLock guard(m_Lock);
        auto       it = m_ExtendedAttributes.Find(String(name));
        if (it == m_ExtendedAttributes.end()) return Error(ENODATA);
        data = it->Value;
    }

    // Like getxattr(2): size 0 asks for the value's size, a buffer that is
    // too small fails, otherwise the value's size is returned
    if (size == 0) return static_cast<isize>(data.Size());
    if (size < data.Size()) return Error(ERANGE);

    if (data.Size() && !CopyToUser(value, data.Raw(), data.Size()))
        return Error(EFAULT);
    return static_cast<isize>(data.Size());
}
ErrorOr<isize>
SynthFsINode::ListExtendedAttributes(::Ref<DirectoryEntry> dentry, char* list,
                                     usize size)
{
    return Error(ENOSYS);
}
ErrorOr<isize>
SynthFsINode::RemoveExtendedAttribute(::Ref<DirectoryEntry> dentry,
                                      StringView            name)
{
    ScopedLock guard(m_Lock);
    auto       it = m_ExtendedAttributes.Find(String(name));
    if (it == m_ExtendedAttributes.end()) return Error(ENODATA);

    m_ExtendedAttributes.Erase(it);
    return 0;
}

ErrorOr<void> SynthFsINode::ResizeBuffer(usize newSize)
{
    m_Buffer.Resize(newSize);
    m_Metadata.Size       = m_Buffer.Size();
    m_Metadata.BlockCount = Math::DivRoundUp(newSize, 512);

    return {};
}
