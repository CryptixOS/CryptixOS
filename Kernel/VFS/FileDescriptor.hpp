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

#include <deque>

enum class FileAccessMode
{
    eNone  = 0,
    eRead  = Bit(0),
    eWrite = Bit(1),
    eExec  = Bit(2),
};
inline bool operator&(const FileAccessMode lhs, FileAccessMode rhs)
{
    return static_cast<u64>(lhs) & static_cast<u64>(rhs);
}

struct FileDescriptor
{
    bool DirentsInvalid = false;

    FileDescriptor(INode* node, i32 flags, mode_t mode)
        : m_Node(node)
    {
        if ((flags & O_ACCMODE) == 0) flags |= O_RDONLY;

        mode &= (S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX | S_ISUID | S_ISGID);

        auto   follow  = !(flags & O_NOFOLLOW);

        auto   acc     = flags & O_ACCMODE;

        size_t accmode = 0;
        switch (acc)
        {
            case O_RDONLY: // accmode = stat_t::read; break;
            case O_WRONLY: // accmode = stat_t::write; break;
            case O_RDWR:   // accmode = stat_t::read | stat_t::write; break;
            default:
        }

        return_err(, EINVAL);

        (void)follow;
        (void)accmode;
        (void)mode;

        // if (flags & o_tmpfile && !(accmode & stat_t::write))
        //     return_err(-1, EINVAL);
    }

    inline INode* GetNode() const { return m_Node; }
    inline usize  GetOffset() const { return m_Offset; }

    isize         Read(void* const outBuffer, usize count);
    isize         Seek(i32 whence, off_t offset);

    void          Lock() { m_Lock.Acquire(); }
    void          Unlock() { m_Lock.Release(); }

    void          Close() {}

    inline bool CanRead() const { return m_AccessMode & FileAccessMode::eRead; }

    inline std::deque<dirent*>& GetDirEntries() { return m_DirEntries; }
    bool                        GenerateDirEntries();

  private:
    Spinlock            m_Lock;
    INode*              m_Node       = nullptr;
    usize               m_Offset     = 0;

    FileAccessMode      m_AccessMode = FileAccessMode::eRead;
    std::deque<dirent*> m_DirEntries;
};
