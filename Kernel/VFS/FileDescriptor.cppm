/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
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
#include <Prism/Containers/Deque.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:VFS.FileDescriptor;

export import :Library.UserBuffer;
export import :VFS.INode;
export import :VFS.File;

export enum class FileAccessMode
{
    eNone  = 0,
    eRead  = Bit(0),
    eWrite = Bit(1),
    eExec  = Bit(2),
};

export constexpr inline FileAccessMode& operator|=(FileAccessMode&      lhs,
                                            const FileAccessMode rhs)
{
    usize ret = static_cast<usize>(lhs) | static_cast<usize>(rhs);

    lhs       = static_cast<FileAccessMode>(ret);
    return lhs;
}
export constexpr inline FileAccessMode operator|(const FileAccessMode lhs,
                                          FileAccessMode       rhs)
{
    usize ret = static_cast<u64>(lhs) | static_cast<u64>(rhs);

    return static_cast<FileAccessMode>(ret);
}
export constexpr inline bool operator&(const FileAccessMode lhs, FileAccessMode rhs)
{
    return static_cast<u64>(lhs) & static_cast<u64>(rhs);
}

export struct DirectoryEntries
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

export enum class FileMode : u64
{
    eNone   = 0,
    eAppend = O_APPEND,
    eAsync  = O_ASYNC,
    eDirect = O_DIRECT,
    ePath   = O_PATH,
};
export class FileDescriptor : public RefCounted
{
  public:
    FileDescriptor(::Ref<DirectoryEntry> dentry, i32 flags,
                   FileAccessMode accMode);
    FileDescriptor(::Ref<DirectoryEntry> dentry, class File* file, i32 flags,
                   FileAccessMode accMode);
    virtual ~FileDescriptor();

    inline ::Ref<INode>   INode() const { return m_DirectoryEntry->INode(); }
    ::Ref<DirectoryEntry> DirectoryEntry() const { return m_DirectoryEntry; }
    class File*           File() const { return m_File; }
    inline usize          GetOffset() const { return m_Offset; }

    inline i32            GetFlags() const { return m_Flags; }
    inline void           SetFlags(i32 flags)
    {
        ScopedLock guard(m_Lock);
        m_Flags = flags;
    }

    inline i32  GetDescriptionFlags() const { return m_Flags; }
    inline void SetDescriptionFlags(i32 flags)
    {
        ScopedLock guard(m_Lock);
        m_Flags = flags;
    }

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1);
    inline ErrorOr<isize>  Read(void* const outBuffer, usize count)
    {
        auto bufferOr = UserBuffer::ForUserBuffer(outBuffer, count);
        if (!bufferOr) return Error(bufferOr.Error());
        auto buffer = bufferOr.Value();

        return Read(buffer, count, m_Offset);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1);
    inline ErrorOr<isize>  Write(const void* inBuffer, usize count)
    {
        auto bufferOr = UserBuffer::ForUserBuffer(inBuffer, count);
        if (!bufferOr) return Error(bufferOr.Error());
        auto buffer = bufferOr.Value();

        return Write(buffer, count, m_Offset);
    }
    ErrorOr<isize>              IoCtl(usize request, usize opcode);
    virtual ErrorOr<const stat> Stat() const;
    virtual ErrorOr<isize>      Seek(i32 whence, off_t offset);
    virtual ErrorOr<isize>      Truncate(off_t size);

    void                        Lock() { m_Lock.Acquire(); }
    void                        Unlock() { m_Lock.Release(); }

    // TODO(v1t10l7): verify whether the fd is blocking
    inline bool                 WouldBlock() const { return false; }

    virtual bool                IsCharDevice();
    virtual bool                IsFifo();
    virtual bool                IsDirectory();
    virtual bool                IsRegular();
    virtual bool                IsSymlink();
    bool                        IsSocket();
    inline bool                 IsPipe()
    {
        // FIXME(v1tr10l7): implement this once pipes are supported

        return false;
    }
    inline bool CanRead() const { return m_AccessMode & FileAccessMode::eRead; }
    inline bool CanWrite() const
    {
        return m_AccessMode & FileAccessMode::eWrite;
    }

    inline bool IsNonBlocking() const { return m_Flags & O_NONBLOCK; }

    inline bool CloseOnExec() const { return m_Flags & O_CLOEXEC; }
    inline void SetCloseOnExec(bool closeOnExec)
    {
        if (closeOnExec) m_Flags |= O_CLOEXEC;
        else m_Flags &= ~O_CLOEXEC;
    }

    [[clang::no_sanitize("alignment")]]
    ErrorOr<isize> GetDirEntries(dirent* const out, usize maxSize);

  private:
    Spinlock                 m_Lock;
    ::Ref<::DirectoryEntry>  m_DirectoryEntry   = nullptr;
    class File*              m_File             = nullptr;

    FileAccessMode           m_AccessMode       = FileAccessMode::eRead;
    usize                    m_Offset           = 0;

    i32                      m_Flags            = 0;
    i32                      m_DescriptionFlags = 0;

    DirectoryEntries         m_DirEntries;
    DirectoryEntry::Iterator m_DirectoryIterator;

    inline DirectoryEntries& GetDirEntries() { return m_DirEntries; }
    bool                     GenerateDirEntries();
};
