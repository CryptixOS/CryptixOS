/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/sys/statfs.h>
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
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/String/String.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
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
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <VFS/TmpFs/TmpFsINode.hpp>

export module Cryptix:VFS.TmpFs.TmpFs;

export import :VFS.Filesystem;
export import :VFS.INode;
export import :VFS.VFS;
export import :VFS.TmpFs.TmpFsINode;

export class TmpFs : public Filesystem
{
  public:
    explicit TmpFs(u32 flags);

    inline usize GetSize() const { return m_Size; }
    inline usize GetMaxSize() const { return m_MaxSize; }

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;

    virtual ErrorOr<::Ref<INode>> AllocateNode(StringView name,
                                               INodeMode  mode) override;
    virtual ErrorOr<void>         FreeINode(::Ref<INode> inode) override;

    virtual bool Populate(DirectoryEntry* dentry) override { return true; }
    virtual ErrorOr<void> Stats(statfs& stats) override;
    constexpr usize       FreeINodeCount() const { return m_FreeINodeCount; }

  private:
    usize         m_MaxBlockCount  = 0;
    Atomic<usize> m_UsedBlockCount = 0;
    usize         m_MaxINodeCount  = 0;
    Atomic<usize> m_FreeINodeCount = PMM::PAGE_SIZE << 2;
    usize         m_MaxSize        = 0;

    usize         m_Size           = 0;

    friend class TmpFsINode;
};
