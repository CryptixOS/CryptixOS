/*
 * Created by v1tr10l7 on 12.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
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
#include <Prism/Core/TypeTraits.hpp>
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

export module Cryptix:VFS.FilesystemView;

export import :Library.Locking.SequenceLock;
export import :VFS.INode;

export namespace VFS
{
    Ref<DirectoryEntry> RootDirectoryEntry();
}

export class FilesystemView : public RefCounted
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
