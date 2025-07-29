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
        m_Metadata.GID    = creds.gid;
        m_Metadata.UID    = creds.uid;
    }

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
}

ErrorOr<void>
SynthFsINode::TraverseDirectories(Ref<class DirectoryEntry> parent,
                                  DirectoryIterator         iterator)
{
    ScopedLock guard(m_Lock);
    usize      offset = 0;
    for (const auto [name, inode] : Children())
    {
        usize  ino  = inode->ID();
        mode_t mode = inode->Mode();
        auto   type = IF2DT(mode);

        if (!iterator(name, offset, ino, type)) break;
        ++offset;
    }

    return {};
}
ErrorOr<Ref<DirectoryEntry>> SynthFsINode::Lookup(Ref<DirectoryEntry> entry)
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

ErrorOr<Ref<DirectoryEntry>> SynthFsINode::CreateNode(Ref<DirectoryEntry> entry,
                                                      mode_t mode, dev_t dev)
{
    ScopedLock guard(m_Lock);
    if (m_Children.Contains(entry->Name())) return Error(EEXIST);

    auto inode = reinterpret_cast<SynthFsINode*>(
        TryOrRet(m_Filesystem->AllocateNode(entry->Name(), mode)));

    inode->m_Parent = this;
    if (S_ISREG(mode))
    {
        inode->m_Metadata.Size = inode->GetDefaultSize();
        inode->m_Buffer.Resize(GetDefaultSize());
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
ErrorOr<Ref<DirectoryEntry>> SynthFsINode::CreateFile(Ref<DirectoryEntry> entry,
                                                      mode_t              mode)
{
    return CreateNode(entry, (mode & ~S_IFMT) | S_IFREG, 0);
}
ErrorOr<Ref<DirectoryEntry>>
SynthFsINode::CreateDirectory(Ref<DirectoryEntry> entry, mode_t mode)
{

    TryOrRet(CreateNode(entry, (mode & ~S_IFMT) | S_IFDIR, 0));

    ++m_Metadata.LinkCount;
    return entry;
}
ErrorOr<Ref<DirectoryEntry>> SynthFsINode::Symlink(Ref<DirectoryEntry> entry,
                                                   PathView targetPath)
{
    auto dentry     = TryOrRet(CreateNode(entry, 0777 | S_IFLNK, 0));
    auto inode      = reinterpret_cast<SynthFsINode*>(dentry->INode());

    inode->m_Target = targetPath;
    return dentry;
}
ErrorOr<Ref<DirectoryEntry>> SynthFsINode::Link(Ref<DirectoryEntry> oldEntry,
                                                Ref<DirectoryEntry> entry)
{
    auto inode = reinterpret_cast<SynthFsINode*>(oldEntry->INode());
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

void SynthFsINode::InsertChild(INode* inode, StringView name)
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

    usize      newCapacity = m_Buffer.Size() ?: PMM::PAGE_SIZE;
    while (offset + bytes > newCapacity) newCapacity *= 2;

    ResizeBuffer(newCapacity);

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

ErrorOr<void> SynthFsINode::Rename(INode* newParent, StringView newName)
{
    // TODO(v1tr10l7): Remove old inode

    // auto parent = reinterpret_cast<TmpFsINode*>(m_Parent);
    // parent->m_Children.Erase(m_Name);

    m_Name = newName;
    newParent->InsertChild(this, Name());

    return {};
}

ErrorOr<void> SynthFsINode::Unlink(Ref<DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);
    auto       it = m_Children.Find(entry->Name());
    if (it == m_Children.end()) return {};

    auto inode = reinterpret_cast<SynthFsINode*>(entry->INode());
    m_Metadata.Size -= DIRECTORY_ENTRY_SIZE;

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};
    UpdateTimestamps(atime, currentTime, currentTime);
    inode->m_Metadata.ChangeTime = currentTime;
    --inode->m_Metadata.LinkCount;

    if (inode->m_Metadata.LinkCount == 0)
    {
        m_Children.Erase(it);
        delete entry->INode();
    }

    auto parent = entry->Parent();
    parent->RemoveChild(entry);

    LogTrace("VFS::TmpFs: Unlinked inode => `{}`, link count => {}",
             entry->Name(), inode->m_Metadata.LinkCount);
    return {};
}
ErrorOr<void> SynthFsINode::RmDir(Ref<DirectoryEntry> entry)
{
    m_Lock.Acquire();
    --m_Metadata.LinkCount;
    m_Lock.Release();

    return Unlink(entry);
}

ErrorOr<void> SynthFsINode::ResizeBuffer(usize newSize)
{
    m_Buffer.Resize(newSize);
    m_Metadata.Size       = m_Buffer.Size();
    m_Metadata.BlockCount = Math::DivRoundUp(newSize, 512);

    return {};
}
