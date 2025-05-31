/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

#include <Drivers/Storage/StorageDevice.hpp>
#include <Library/Spinlock.hpp>

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

namespace NVMe
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
        NameSpace(u32 id, class Controller* controller);
        virtual ~NameSpace() = default;

        bool               Initialize();

        inline usize       GetMaxPhysRPgs() const { return m_MaxPhysRPages; }

        virtual StringView GetName() const noexcept override;

        virtual ErrorOr<isize> Read(void* dest, off_t offset,
                                    usize bytes) override;
        virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                     usize bytes) override;

        virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                    isize offset = -1) override;
        virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                     isize offset = -1) override;

        virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }

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

        isize ReadWriteLba(u8* ddest, usize start, usize bytes, u8 write);

        isize FindBlock(u64 block);
        isize CacheBlock(u64 block);
    };
}; // namespace NVMe
