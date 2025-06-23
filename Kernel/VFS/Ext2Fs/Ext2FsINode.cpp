/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Ext2Fs/Ext2FsINode.hpp>

#include <Time/Time.hpp>

Ext2FsINode::Ext2FsINode(INode* parent, StringView name, Ext2Fs* fs,
                         mode_t mode)
    : INode(parent, name, fs)
    , m_Fs(fs)
{
    m_Stats.st_dev     = fs->GetDeviceID();
    m_Stats.st_ino     = fs->GetNextINodeIndex();
    m_Stats.st_mode    = mode;
    m_Stats.st_nlink   = 1;
    m_Stats.st_uid     = 0;
    m_Stats.st_gid     = 0;
    m_Stats.st_rdev    = 0;
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = m_Fs->GetBlockSize();
    m_Stats.st_blocks  = 0;

    m_Stats.st_atim    = Time::GetReal();
    m_Stats.st_ctim    = Time::GetReal();
    m_Stats.st_mtim    = Time::GetReal();
}

INode* Ext2FsINode::Lookup(const String& name)
{
    Ext2FsINodeMeta meta;
    m_Fs->ReadINodeEntry(&meta, m_Stats.st_ino);

    usize pageCount = Math::DivRoundUp(meta.GetSize(), PMM::PAGE_SIZE);
    u8*   buffer = Pointer(PMM::CallocatePages(pageCount)).ToHigherHalf<u8*>();
    m_Fs->ReadINode(meta, buffer, 0, meta.GetSize());

    for (usize i = 0; i < meta.GetSize();)
    {
        Ext2FsDirectoryEntry* entry
            = reinterpret_cast<Ext2FsDirectoryEntry*>(buffer + i);

        char* nameBuffer = new char[entry->NameSize + 1];
        std::strncpy(nameBuffer, reinterpret_cast<char*>(entry->Name),
                     entry->NameSize);
        nameBuffer[entry->NameSize] = 0;

        if (std::strncmp(nameBuffer, name.Raw(), name.Size()) != 0)
        {
            i += entry->Size;
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

        Ext2FsINode* newNode    = new Ext2FsINode(this, nameBuffer, m_Fs, mode);
        newNode->m_Stats.st_uid = inodeMeta.UID;
        newNode->m_Stats.st_gid = inodeMeta.GID;
        newNode->m_Stats.st_ino = entry->INodeIndex;
        newNode->m_Stats.st_size  = inodeMeta.GetSize();
        newNode->m_Stats.st_nlink = inodeMeta.HardLinkCount;
        newNode->m_Stats.st_blocks
            = newNode->m_Stats.st_size / m_Fs->GetBlockSize();

        newNode->m_Stats.st_atim.tv_sec  = inodeMeta.AccessTime;
        newNode->m_Stats.st_atim.tv_nsec = 0;
        newNode->m_Stats.st_ctim.tv_sec  = inodeMeta.CreationTime;
        newNode->m_Stats.st_ctim.tv_nsec = 0;
        newNode->m_Stats.st_mtim.tv_sec  = inodeMeta.ModifiedTime;
        newNode->m_Stats.st_mtim.tv_nsec = 0;

        newNode->m_Populated             = false;
        newNode->m_Meta                  = inodeMeta;
        InsertChild(newNode, newNode->GetName());

        // TODO(v1tr10l7): Resolve symlink
        if (newNode->IsSymlink())
            ;

        delete[] nameBuffer;
        PMM::FreePages(Pointer(buffer).FromHigherHalf(), pageCount);
        return newNode;
    }

    return nullptr;
}

void Ext2FsINode::InsertChild(INode* node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}
isize Ext2FsINode::Read(void* buffer, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);
    m_Fs->ReadINodeEntry(&m_Meta, m_Stats.st_ino);

    if (static_cast<isize>(offset + bytes) > m_Stats.st_size)
        bytes = bytes - ((offset + bytes) - m_Stats.st_size);

    m_Stats.st_atim   = Time::GetReal();
    m_Meta.AccessTime = m_Stats.st_atim.tv_sec;
    m_Fs->WriteINodeEntry(m_Meta, m_Stats.st_ino);

    return m_Fs->ReadINode(m_Meta, reinterpret_cast<u8*>(buffer), offset,
                           bytes);
}

ErrorOr<void> Ext2FsINode::ChMod(mode_t mode)
{
    ScopedLock guard(m_Lock);
    m_Fs->ReadINodeEntry(&m_Meta, m_Stats.st_ino);

    m_Meta.Permissions &= ~0777;
    m_Meta.Permissions |= mode & 0777;

    m_Fs->WriteINodeEntry(m_Meta, m_Stats.st_ino);

    m_Stats.st_mode &= ~0777;
    m_Stats.st_mode |= mode & 0777;

    return {};
}

void Ext2FsINode::Initialize(ino_t index, mode_t mode, u16 type)
{
    m_Stats.st_dev                  = m_Fs->GetDeviceID();
    m_Stats.st_ino                  = index;
    m_Stats.st_nlink                = 1;
    m_Stats.st_mode                 = mode;
    // TODO(v1tr10l7): Credentials
    m_Stats.st_uid                  = 0;
    m_Stats.st_gid                  = 0;
    m_Stats.st_rdev                 = 0;
    m_Stats.st_size                 = 0;
    m_Stats.st_blksize              = m_Fs->GetBlockSize();
    m_Stats.st_blocks               = 0;
    // TODO(v1tr10l7): atim, mtim, ctim

    m_Meta.Permissions              = (mode & 0xfff) | type;
    m_Meta.UID                      = m_Stats.st_uid;
    m_Meta.SizeLow                  = 0;
    // TODO(v1tr10l7): Update UTimes
    m_Meta.AccessTime               = 0;
    m_Meta.CreationTime             = 0;
    m_Meta.ModifiedTime             = 0;
    m_Meta.DeletedTime              = 0;
    m_Meta.GID                      = m_Stats.st_gid;
    m_Meta.HardLinkCount            = m_Stats.st_nlink;
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
    m_Fs->ReadINodeEntry(&m_Meta, m_Stats.st_ino);

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

            m_Fs->WriteINode(m_Meta, buffer, m_Stats.st_ino, 0,
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

    m_Fs->WriteINode(m_Meta, buffer, m_Stats.st_ino, 0, m_Meta.GetSize());
    PMM::FreePages(buffer, pageCount);

    return {};
}
