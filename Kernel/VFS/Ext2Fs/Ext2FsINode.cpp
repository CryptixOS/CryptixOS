/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/dirent.h>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Ext2Fs/Ext2FsINode.hpp>

#include <Time/Time.hpp>

Ext2FsINode::Ext2FsINode(StringView name, Ext2Fs* fs, mode_t mode)
    : INode(name, fs)
    , m_Fs(fs)
{
    m_Metadata.DeviceID         = fs->DeviceID();
    m_Metadata.ID               = fs->NextINodeIndex();
    m_Metadata.Mode             = mode;
    m_Metadata.LinkCount        = 1;
    m_Metadata.UID              = 0;
    m_Metadata.GID              = 0;
    m_Metadata.RootDeviceID     = 0;
    m_Metadata.Size             = 0;
    m_Metadata.BlockSize        = m_Fs->GetBlockSize();
    m_Metadata.BlockCount       = 0;

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();
}

ErrorOr<void> Ext2FsINode::TraverseDirectories(class DirectoryEntry* parent,
                                               DirectoryIterator     iterator)
{
    LogTrace("Ext2fs: Traversing directories");
    m_Fs->ReadINodeEntry(&m_Meta, m_Metadata.ID);

    u8* buffer = new u8[m_Meta.GetSize()];
    m_Fs->ReadINode(m_Meta, buffer, 0, m_Meta.GetSize());

    usize bufferOffset = 0;
    usize i            = 0;

    LogTrace("Ext2Fs: Reading children directory entries of {}", Name());
    for (bufferOffset = 0, i = 0; bufferOffset < m_Meta.GetSize(); i++)
    {
        Ext2FsDirectoryEntry* entry
            = reinterpret_cast<Ext2FsDirectoryEntry*>(buffer + bufferOffset);
        if (i < m_DirectoryOffset) continue;

        char* nameBuffer = new char[entry->NameSize + 1];
        std::strncpy(nameBuffer, reinterpret_cast<char*>(entry->Name),
                     entry->NameSize);
        nameBuffer[entry->NameSize] = 0;
        if (entry->INodeIndex == 0)
        {
            delete[] nameBuffer;
            bufferOffset += entry->Size;
            continue;
        }
        if (!std::strcmp(nameBuffer, ".") && !std::strcmp(nameBuffer, ".."))
        {
            bufferOffset += entry->Size;
            continue;
        }

        Ext2FsINodeMeta inodeMeta;
        m_Fs->ReadINodeEntry(&inodeMeta, entry->INodeIndex);

        u64 mode = (inodeMeta.Permissions & 0xfff);
        switch (entry->Type)
        {
            case Ext2FsDirectoryEntryType::eRegular: mode |= S_IFREG; break;
            case Ext2FsDirectoryEntryType::eDirectory: mode |= S_IFDIR; break;
            case Ext2FsDirectoryEntryType::eCharacterDevice:
                mode |= S_IFCHR;
                break;
            case Ext2FsDirectoryEntryType::eBlockDevice: mode |= S_IFBLK; break;
            case Ext2FsDirectoryEntryType::eFifo: mode |= S_IFIFO; break;
            case Ext2FsDirectoryEntryType::eSocket: mode |= S_IFSOCK; break;
            case Ext2FsDirectoryEntryType::eSymlink: mode |= S_IFLNK; break;

            default:
                LogError("Ext2Fs: Invalid directory entry type: {}",
                         ToUnderlying(entry->Type));
                break;
        }

        Ext2FsINode* newNode          = new Ext2FsINode(nameBuffer, m_Fs, mode);
        newNode->m_Metadata.UID       = inodeMeta.UID;
        newNode->m_Metadata.GID       = inodeMeta.GID;
        newNode->m_Metadata.ID        = entry->INodeIndex;
        newNode->m_Metadata.Size      = inodeMeta.GetSize();
        newNode->m_Metadata.LinkCount = inodeMeta.HardLinkCount;
        newNode->m_Metadata.BlockCount
            = newNode->m_Metadata.Size / m_Fs->GetBlockSize();

        newNode->m_Metadata.AccessTime.tv_sec        = inodeMeta.AccessTime;
        newNode->m_Metadata.AccessTime.tv_nsec       = 0;
        newNode->m_Metadata.ChangeTime.tv_sec        = inodeMeta.CreationTime;
        newNode->m_Metadata.ChangeTime.tv_nsec       = 0;
        newNode->m_Metadata.ModificationTime.tv_sec  = inodeMeta.ModifiedTime;
        newNode->m_Metadata.ModificationTime.tv_nsec = 0;

        newNode->m_Meta                              = inodeMeta;
        LogTrace(
            "Ext2Fs: New Ext2FsINode =>\n"
            "\tname => {}\n"
            "\tid => {}\n"
            "index => {}\n",
            newNode->Name(), newNode->m_Metadata.ID, i);

        InsertChild(newNode, newNode->Name());

        // TODO(v1tr10l7): resolve link
        if (newNode->IsSymlink())
            ;

        delete[] nameBuffer;
        bufferOffset += entry->Size;

        auto type   = IF2DT(mode);
        auto offset = m_DirectoryOffset;
        ++m_DirectoryOffset;

        if (!iterator(newNode->Name(), offset, newNode->m_Metadata.ID, type))
            break;
    }

    if (bufferOffset == m_Meta.GetSize()) m_DirectoryOffset = 0;
    delete[] buffer;

    return {};
}
ErrorOr<Ref<DirectoryEntry>> Ext2FsINode::Lookup(Ref<DirectoryEntry> dentry)
{
    auto iterator
        = [&](StringView name, loff_t offset, usize ino, usize type) -> bool
    {
        if (name == dentry->Name()) return false;

        return true;
    };

    Delegate<bool(StringView, loff_t, usize, usize)> delegate;
    delegate.BindLambda(iterator);

    LogTrace("Ext2Fs: Looking up an inode => `{}`", dentry->Name());
    TraverseDirectories(dentry->Parent().Raw(), delegate);

    for (const auto& [name, inode] : Children())
    {
        if (name != inode->Name()) continue;

        dentry->Bind(inode);
        return dentry;
    }

    return Error(ENOENT);
}

void Ext2FsINode::InsertChild(INode* node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}
isize Ext2FsINode::Read(void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);
    m_Fs->ReadINodeEntry(&m_Meta, m_Metadata.ID);

    if (offset + bytes > m_Metadata.Size)
        bytes = bytes - ((offset + bytes) - m_Metadata.Size);

    m_Metadata.AccessTime = Time::GetReal();
    m_Meta.AccessTime     = m_Metadata.AccessTime.tv_sec;
    m_Fs->WriteINodeEntry(m_Meta, m_Metadata.ID);

    return m_Fs->ReadINode(m_Meta, reinterpret_cast<u8*>(buffer), offset,
                           bytes);
}

/*
ErrorOr<void> Ext2FsINode::ChMod(mode_t mode)
{
    ScopedLock guard(m_Lock);
    m_Fs->ReadINodeEntry(&m_Meta, m_Metadata.ID);

    m_Meta.Permissions &= ~0777;
    m_Meta.Permissions |= mode & 0777;

    m_Fs->WriteINodeEntry(m_Meta, m_Metadata.ID);

    m_Metadata.Mode &= ~0777;
    m_Metadata.Mode |= mode & 0777;

    return {};
}
*/

void Ext2FsINode::Initialize(ino_t index, mode_t mode, u16 type)
{
    m_Metadata.DeviceID             = m_Fs->DeviceID();
    m_Metadata.ID                   = index;
    m_Metadata.LinkCount            = 1;
    m_Metadata.Mode                 = mode;
    // TODO(v1tr10l7): Credentials
    m_Metadata.UID                  = 0;
    m_Metadata.GID                  = 0;
    m_Metadata.RootDeviceID         = 0;
    m_Metadata.Size                 = 0;
    m_Metadata.BlockSize            = m_Fs->GetBlockSize();
    m_Metadata.BlockCount           = 0;
    // TODO(v1tr10l7): atim, mtim, ctim

    m_Meta.Permissions              = (mode & 0xfff) | type;
    m_Meta.UID                      = m_Metadata.UID;
    m_Meta.SizeLow                  = 0;
    // TODO(v1tr10l7): Update UTimes
    m_Meta.AccessTime               = 0;
    m_Meta.CreationTime             = 0;
    m_Meta.ModifiedTime             = 0;
    m_Meta.DeletedTime              = 0;
    m_Meta.GID                      = m_Metadata.GID;
    m_Meta.HardLinkCount            = m_Metadata.LinkCount;
    m_Meta.SectorCount              = 0;
    m_Meta.Flags                    = 0;
    m_Meta.OperatingSystemSpecific1 = 0;
    for (usize i = 0; i < 15; i++) m_Meta.Blocks[i] = 0;
    // FIXME(v1tr10l7): Randomly generate
    m_Meta.GenerationNumber       = 0;
    m_Meta.ExtendedAttributeBlock = 0;
    m_Meta.SizeHigh               = 0;
    m_Meta.FragmentAddress        = 0;
    std::memset(m_Meta.OperatingSystemSpecific2, 0,
                sizeof(m_Meta.OperatingSystemSpecific2));
}
ErrorOr<void> Ext2FsINode::AddDirectoryEntry(Ext2FsDirectoryEntry& dentry)
{
    m_Fs->ReadINodeEntry(&m_Meta, m_Metadata.ID);

    usize pageCount = Math::DivRoundUp(m_Meta.GetSize(), PMM::PAGE_SIZE);
    auto  buffer = Pointer(PMM::CallocatePages(pageCount)).ToHigherHalf<u8*>();
    m_Fs->ReadINode(m_Meta, buffer, 0, m_Meta.GetSize());

    usize nameSize  = std::strlen(reinterpret_cast<char*>(dentry.Name));
    usize required  = (sizeof(Ext2FsDirectoryEntry) + nameSize + 3) & ~3;
    dentry.NameSize = nameSize;

    usize offset    = 0;
    for (; offset < m_Meta.GetSize();)
    {
        Ext2FsDirectoryEntry* previousEntry
            = reinterpret_cast<Ext2FsDirectoryEntry*>(buffer + offset);

        usize contracted
            = (sizeof(Ext2FsDirectoryEntry) + previousEntry->NameSize + 3) & ~3;
        usize available = previousEntry->Size - contracted;

        if (available >= required)
        {
            previousEntry->Size = contracted;
            auto entry = reinterpret_cast<Ext2FsDirectoryEntry*>(buffer + offset
                                                                 + contracted);
            std::memset(entry, 0, entry->Size);

            entry->INodeIndex = dentry.INodeIndex;
            entry->Size       = available;
            entry->NameSize   = nameSize;
            entry->Type       = dentry.Type;
            std::strncpy(reinterpret_cast<char*>(entry->Name),
                         reinterpret_cast<char*>(dentry.Name), nameSize + 1);

            m_Fs->WriteINode(m_Meta, buffer, m_Metadata.ID, 0,
                             m_Meta.GetSize());
            PMM::FreePages(Pointer(buffer).FromHigherHalf(), pageCount);
            return {};
        }

        offset += previousEntry->Size;
    }

    PMM::FreePages(Pointer(buffer).FromHigherHalf(), pageCount);

    Ext2FsINodeMeta newEntry;
    m_Fs->ReadINodeEntry(&newEntry, dentry.INodeIndex);
    m_Fs->GrowINode(newEntry, dentry.INodeIndex, 0,
                    m_Meta.GetSize() + m_Fs->GetBlockSize());
    m_Fs->WriteINodeEntry(newEntry, dentry.INodeIndex);

    pageCount = Math::DivRoundUp(m_Meta.GetSize(), PMM::PAGE_SIZE);
    buffer    = Pointer(PMM::CallocatePages(pageCount));
    m_Fs->ReadINode(m_Meta, buffer, 0, m_Meta.GetSize());

    Ext2FsDirectoryEntry* entry
        = reinterpret_cast<Ext2FsDirectoryEntry*>(buffer + offset);
    std::memset(entry, 0, sizeof(*entry));
    entry->INodeIndex = dentry.INodeIndex;
    entry->Size       = newEntry.GetSize() - offset;
    entry->NameSize   = nameSize;
    entry->Type       = dentry.Type;
    std::strncpy(reinterpret_cast<char*>(entry->Name),
                 reinterpret_cast<char*>(dentry.Name), nameSize + 1);

    m_Fs->WriteINode(m_Meta, buffer, m_Metadata.ID, 0, m_Meta.GetSize());
    PMM::FreePages(buffer, pageCount);

    return {};
}
