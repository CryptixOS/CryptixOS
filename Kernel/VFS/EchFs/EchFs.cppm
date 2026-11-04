/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <VFS/EchFs/EchFsStructures.hpp>
#include <API/Posix/termios.h>
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
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
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
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <API/Posix/sys/statfs.h>

export module Cryptix:VFS.EchFs.EchFs;

export import :VFS.EchFs.EchFsStructures;
export import :VFS.Filesystem;
import :VFS.EchFs.EchFsINode;

export class EchFsINode;
export class EchFs final : public Filesystem
{
  public:
    explicit EchFs(u32 flags);
    virtual ~EchFs();

    virtual ErrorOr<::Ref<DirectoryEntry>>
         Mount(StringView sourcePath, const void* data = nullptr) override;
    bool Populate(EchFsINode* inode);
    virtual bool Populate(DirectoryEntry* dentry) override;

    isize ReadDirectoryEntry(EchFsDirectoryEntry& entry, u8* dest, isize offset,
                             usize bytes);

  private:
    EchFsIdentityTable* m_IdentityTable         = nullptr;
    ::Ref<EchFsINode>   m_NativeRoot            = nullptr;

    usize               m_AllocationTableOffset = 0;
    usize               m_AllocationTableSize   = 0;

    usize               m_MainDirectoryStart    = 0;
    usize               m_MainDirectoryEnd      = 0;
};
