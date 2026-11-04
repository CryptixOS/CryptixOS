/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
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
#include <Prism/Memory/Scope.hpp>

export module Cryptix:Drivers.Storage.NVMe.NVMeNameSpace;

export import :Drivers.Storage.StorageDevice;
export import :Library.Locking.Spinlock;

export namespace NVMe
{
    struct LBA
    {
        u16 MetaSize;
        u8  DataSize;
        u8  RelativePerformance;
    };
    struct NameSpaceInfo
    {
        u64 TotalBlockCount;
        u64 MaxCapacity;
        u64 AllocatedBlockCount;
        u8  Features;
        u8  LbaFormatCount;
        u8  FormattedLbaSize;
        u8  MetadataCapabilities;
        u8  EndToEndProtCapabilities;
        u8  EndToEndProtTypeSettings;
        u8  MultiPathAndSharingCaps;
        u8  ReservationCapabilities;
        u8  FormatProgressIndicator;
        u8  Unused1;
        u16 AtomicWriteUnitNormal;
        u16 AtomicWriteUnitPowerFail;
        u16 AtomicCompareAndWriteUnit;
        u16 AtomicBoundarySize;
        u16 FirstAtomicBoundaryLba;
        u16 AtomicWriteUnitPowerFailBoundSize;
        u16 Unused2;
        u64 NvmCapacity[2];
        u64 Unused3[5];
        u8  GuID[16];
        u8  ExtendedUID[8];
        LBA LbaFormatUpper[16];
        u64 Unused4[24];
        u8  VendorSpecific[3712];
    };

    struct CachedBlock
    {
        u8* Cache;
        u64 Block;
        u64 End;
        i32 Status;
    };

    class Queue;
    class NameSpace : public StorageDevice
    {
      public:
        NameSpace(StringView name, u32 id, class Controller* controller);
        virtual ~NameSpace() = default;

        bool         Initialize();

        inline usize GetMaxPhysRPgs() const { return m_MaxPhysRPages; }

        virtual ErrorOr<isize> Read(void* dest, off_t offset,
                                    usize bytes) override;
        virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                     usize bytes) override;

        virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                    isize offset = -1) override;
        virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                     isize offset = -1) override;

        virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override
        {
            return Error(ENOSYS);
        }

      private:
        Spinlock               m_Lock;
        u32                    m_ID            = 0;
        Controller*            m_Controller    = nullptr;
        usize                  m_MaxPhysRPages = 0;
        Queue*                 m_IoQueue;

        u64                    m_LbaSize        = 0;
        u64                    m_LbaCount       = 0;
        u64                    m_CacheBlockSize = 0;
        CachedBlock*           m_Cache          = nullptr;
        usize                  m_Overwritten    = 0;

        static constexpr usize CacheWait        = 0;
        static constexpr usize CacheReady       = 1;
        static constexpr usize CacheDirty       = 2;

        bool                   Identify(NameSpaceInfo* nsid);

        isize ReadWriteLba(u8* ddest, usize start, usize bytes, bool write);

        isize FindBlock(u64 block);
        isize CacheBlock(u64 block);
    };
}; // namespace NVMe
