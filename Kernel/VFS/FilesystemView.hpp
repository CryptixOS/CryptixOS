/*
 * Created by v1tr10l7 on 12.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>
#include <Library/Locking/SequenceLock.hpp>
#include <VFS/DirectoryEntry.hpp>

namespace VFS
{
    Ref<DirectoryEntry> RootDirectoryEntry();
}

class FilesystemView : public RefCounted
{
  public:
    inline FilesystemView() = default;
    inline FilesystemView(const FilesystemView& other)
    {
        m_Root             = other.m_Root;
        m_WorkingDirectory = other.m_WorkingDirectory;
        m_FileCreationMask = other.m_FileCreationMask;
    }

    CTOS_NODISCARD inline ::Ref<DirectoryEntry> Root() const
    {
        ::Ref<DirectoryEntry> result;
        usize                 sequence;

        do {
            sequence = m_Lock.ReadBegin();
            result   = m_Root;
        } while (m_Lock.ReadRetry(sequence));

        return result;
    }
    CTOS_NODISCARD inline ::Ref<DirectoryEntry> WorkingDirectory() const
    {
        ::Ref<DirectoryEntry> result;
        usize                 sequence;

        do {
            sequence = m_Lock.ReadBegin();
            result   = m_WorkingDirectory;
        } while (m_Lock.ReadRetry(sequence));

        return result;
    }
    CTOS_NODISCARD inline INodeMode FileCreationMask() const
    {
        INodeMode result;
        usize     sequence;

        do {
            sequence = m_Lock.ReadBegin();
            result   = m_FileCreationMask;
        } while (m_Lock.ReadRetry(sequence));

        return result;
    }

    inline void SetRoot(::Ref<DirectoryEntry> root)
    {
        SequenceWriterGuard guard(m_Lock, true);
        m_Root = root;
    }
    inline void ChangeDirectory(::Ref<DirectoryEntry> dir)
    {
        SequenceWriterGuard guard(m_Lock, true);
        m_WorkingDirectory = dir;
    }
    inline void SetFileCreationMask(INodeMode mode)
    {
        SequenceWriterGuard guard(m_Lock, true);
        m_FileCreationMask = mode;
    }

  private:
    SequenceLock          m_Lock;
    ::Ref<DirectoryEntry> m_Root             = VFS::RootDirectoryEntry();
    ::Ref<DirectoryEntry> m_WorkingDirectory = VFS::RootDirectoryEntry();
    isize                 m_FileCreationMask = 0;
};
