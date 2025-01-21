/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>

#include <VFS/INode.hpp>

#include <cerrno>
#include <deque>
#include <expected>

enum class FileAccessMode
{
    eNone  = 0,
    eRead  = Bit(0),
    eWrite = Bit(1),
    eExec  = Bit(2),
};

inline FileAccessMode& operator|=(FileAccessMode& lhs, const FileAccessMode rhs)
{
    usize ret = static_cast<usize>(lhs) | static_cast<usize>(rhs);

    lhs       = static_cast<FileAccessMode>(ret);
    return lhs;
}

inline FileAccessMode operator|(const FileAccessMode lhs, FileAccessMode rhs)
{
    usize ret = static_cast<u64>(lhs) | static_cast<u64>(rhs);

    return static_cast<FileAccessMode>(ret);
}
inline bool operator&(const FileAccessMode lhs, FileAccessMode rhs)
{
    return static_cast<u64>(lhs) & static_cast<u64>(rhs);
}

struct DirectoryEntries
{
    using ListType = std::deque<dirent*>;
    ListType    Entries;
    usize       Size             = 0;
    bool        ShouldRegenerate = false;

    dirent*     Front() const { return Entries.front(); }
    inline bool IsEmpty() const { return Entries.empty(); }

    inline void Clear() { Entries.clear(); }
    void        Push(INode* node, std::string_view = "");

    usize       CopyAndPop(u8* out)
    {
        if (Entries.empty()) return 0;

        auto  entry     = Entries.pop_front_element();
        usize entrySize = entry->d_reclen;

        std::memcpy(out, entry, entry->d_reclen);
        delete entry;

        return entrySize;
    }

    auto begin() { return Entries.begin(); }
    auto end() { return Entries.end(); }
};
struct FileDescription
{
    Spinlock           Lock;
    std::atomic<usize> RefCount = 0;

    INode*             Node;
    usize              Offset     = 0;

    i32                Flags      = 0;
    FileAccessMode     AccessMode = FileAccessMode::eRead;
    DirectoryEntries   DirEntries;

    inline void        IncRefCount() { ++RefCount; }
};

struct FileDescriptor
{
    FileDescriptor(INode* node, i32 flags, FileAccessMode accMode);
    FileDescriptor(FileDescriptor* fd, i32 flags = 0);
    ~FileDescriptor();

    inline INode* GetNode() const { return m_Description->Node; }
    inline usize  GetOffset() const { return m_Description->Offset; }

    inline i32    GetFlags() const { return m_Flags; }
    inline void   SetFlags(i32 flags)
    {
        ScopedLock guard(m_Lock);
        m_Flags = flags;
    }

    inline i32  GetDescriptionFlags() const { return m_Description->Flags; }
    inline void SetDescriptionFlags(i32 flags)
    {
        ScopedLock guard(m_Description->Lock);
        m_Description->Flags = flags;
    }

    ErrorOr<isize>       Read(void* const outBuffer, usize count);
    ErrorOr<isize>       Write(const void* data, isize bytes);
    ErrorOr<const stat*> Stat() const;
    isize                Seek(i32 whence, off_t offset);

    void                 Lock() { m_Description->Lock.Acquire(); }
    void                 Unlock() { m_Description->Lock.Release(); }

    inline bool          IsPipe() const
    {
        // FIXME(v1tr10l7): implement this once pipes are supported

        return false;
    }
    inline bool CanRead() const
    {
        return m_Description->AccessMode & FileAccessMode::eRead;
    }
    inline bool CanWrite() const
    {
        return m_Description->AccessMode & FileAccessMode::eWrite;
    }
    inline bool CloseOnExec() const { return m_Flags & O_CLOEXEC; }

    [[clang::no_sanitize("alignment")]]
    ErrorOr<i32> GetDirEntries(dirent* const out, usize maxSize);

  private:
    Spinlock                 m_Lock;
    FileDescription*         m_Description = nullptr;
    i32                      m_Flags       = 0;

    inline DirectoryEntries& GetDirEntries()
    {
        return m_Description->DirEntries;
    }
    bool GenerateDirEntries();
};
