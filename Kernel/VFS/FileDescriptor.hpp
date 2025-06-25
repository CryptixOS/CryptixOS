/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>

#include <Library/UserBuffer.hpp>
#include <Prism/Containers/Deque.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/File.hpp>

enum class FileAccessMode
{
    eNone  = 0,
    eRead  = Bit(0),
    eWrite = Bit(1),
    eExec  = Bit(2),
};

constexpr inline FileAccessMode& operator|=(FileAccessMode&      lhs,
                                            const FileAccessMode rhs)
{
    usize ret = static_cast<usize>(lhs) | static_cast<usize>(rhs);

    lhs       = static_cast<FileAccessMode>(ret);
    return lhs;
}
constexpr inline FileAccessMode operator|(const FileAccessMode lhs,
                                          FileAccessMode       rhs)
{
    usize ret = static_cast<u64>(lhs) | static_cast<u64>(rhs);

    return static_cast<FileAccessMode>(ret);
}
constexpr inline bool operator&(const FileAccessMode lhs, FileAccessMode rhs)
{
    return static_cast<u64>(lhs) & static_cast<u64>(rhs);
}

struct DirectoryEntries
{
    using ListType = Deque<dirent*>;
    ListType    Entries;
    usize       Size             = 0;
    bool        ShouldRegenerate = false;

    dirent*     Front() const { return Entries.Front(); }
    inline bool IsEmpty() const { return Entries.Empty(); }

    inline void Clear() { Entries.Clear(); }
    void        Push(StringView name, loff_t offset, usize ino, usize type);

    usize       CopyAndPop(u8* out, usize capacity);

    auto        begin() { return Entries.begin(); }
    auto        end() { return Entries.end(); }
};
struct FileDescription
{
    Spinlock         Lock;
    Atomic<usize>    RefCount = 0;

    DirectoryEntry*  Entry;
    usize            Offset     = 0;

    i32              Flags      = 0;
    FileAccessMode   AccessMode = FileAccessMode::eRead;
    DirectoryEntries DirEntries;

    inline ~FileDescription()
    {
        // FIXME(v1t10l7): if (RefCount == 0) delete Node;
    }

    inline void IncRefCount() { ++RefCount; }
    inline void DecRefCount() { --RefCount; }
};

class FileDescriptor : public File
{
  public:
    FileDescriptor(DirectoryEntry* node, i32 flags, FileAccessMode accMode);
    FileDescriptor(FileDescriptor* fd, i32 flags = 0);
    virtual ~FileDescriptor();

    inline INode* INode() const { return m_Description->Entry->INode(); }
    constexpr DirectoryEntry* DirectoryEntry() const
    {
        return m_Description->Entry;
    }
    inline usize GetOffset() const { return m_Description->Offset; }

    inline i32   GetFlags() const { return m_Flags; }
    inline void  SetFlags(i32 flags)
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

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override;
    inline ErrorOr<isize>  Read(void* const outBuffer, usize count)
    {
        auto bufferOr = UserBuffer::ForUserBuffer(outBuffer, count);
        if (!bufferOr) return Error(bufferOr.error());
        auto buffer = bufferOr.value();

        return Read(buffer, count, m_Description->Offset);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override;
    inline ErrorOr<isize>  Write(const void* inBuffer, usize count)
    {
        auto bufferOr = UserBuffer::ForUserBuffer(inBuffer, count);
        if (!bufferOr) return Error(bufferOr.error());
        auto buffer = bufferOr.value();

        return Write(buffer, count, m_Description->Offset);
    }
    virtual ErrorOr<const stat*> Stat() const override;
    virtual ErrorOr<isize>       Seek(i32 whence, off_t offset) override;
    virtual ErrorOr<isize>       Truncate(off_t size) override;

    void                         Lock() { m_Description->Lock.Acquire(); }
    void                         Unlock() { m_Description->Lock.Release(); }

    // TODO(v1t10l7): verify whether the fd is blocking
    inline bool                  WouldBlock() const { return false; }
    inline bool IsSocket() const override { return INode()->IsSocket(); }
    inline bool IsPipe() const
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

    inline bool IsNonBlocking() const { return m_Flags & O_NONBLOCK; }
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
