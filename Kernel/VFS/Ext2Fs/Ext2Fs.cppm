/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Prism/Core/Types.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
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
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <API/Posix/sys/statfs.h>

export module Cryptix:VFS.Ext2Fs.Ext2Fs;

export import :VFS.Ext2Fs.Ext2FsAllocator;
export import :VFS.Ext2Fs.Ext2FsStructures;
export import :VFS.File;
export import :VFS.Filesystem;
import :VFS.Ext2Fs.Ext2FsINode;

export class Ext2FsINode;
export class Ext2Fs : public Filesystem
{
  public:
    explicit Ext2Fs(u32 flags);
    virtual ~Ext2Fs() = default;

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;

    virtual ErrorOr<::Ref<INode>> AllocateNode(StringView name = "",
                                               INodeMode  mode = 0) override;
    virtual ErrorOr<void>         FreeINode(::Ref<INode> inode) override;

    virtual ErrorOr<void>         Sync() override;
    virtual bool                  Populate(DirectoryEntry* dentry) override;
    virtual ErrorOr<void>         Stats(statfs& stats) override;

    ErrorOr<::Ref<INode>>         CreateNode(::Ref<INode>          parent,
                                             ::Ref<DirectoryEntry> entry, mode_t mode,
                                             uid_t uid = 0, gid_t gid = 0);

    inline Ext2FsSuperBlock*      GetSuperBlock() const { return m_SuperBlock; }
    inline usize                  GetBlockSize() const { return m_BlockSize; }

    isize SetINodeBlock(Ext2FsINodeMeta& meta, u32 inode, u32 iblock,
                        u32 dblock);
    void  AssignINodeBlocks(Ext2FsINodeMeta& meta, u32 inode, usize start,
                            usize blocks);
    isize GrowINode(Ext2FsINodeMeta& meta, u32 inode, usize start, usize count);

    usize AllocateBlock(Ext2FsINodeMeta& meta, u32 inode);
    void  FreeBlock(usize block);

    void  ReadINodeEntry(Ext2FsINodeMeta* out, u32 index);
    void  WriteINodeEntry(Ext2FsINodeMeta& in, u32 index);
    isize ReadINode(Ext2FsINodeMeta& meta, u8* out, off_t offset, usize bytes);
    isize WriteINode(Ext2FsINodeMeta& meta, u8* in, u32 inode, off_t offset,
                     usize count);

  private:
    File*             m_Device                     = nullptr;
    // u64               m_DeviceID;

    Ext2FsSuperBlock* m_SuperBlock                 = nullptr;
    usize             m_BlockSize                  = 0;
    usize             m_FragmentSize               = 0;
    usize             m_BlockGroupDescriptionCount = 0;
    friend class Ext2FsAllocator;
    Ext2FsAllocator m_Allocator;

    ScopedLock&&    LockSuperBlock();
    void            ReadSuperBlock();
    void            FlushSuperBlock();

    void ReadBlockGroupDescriptor(Ext2FsBlockGroupDescriptor* out, usize index);
    void WriteBlockGroupDescriptor(Ext2FsBlockGroupDescriptor& in, usize index);
    u32  GetINodeBlock(Ext2FsINodeMeta& meta, u32 blockIndex);
};
