/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>
#include <Drivers/Device.hpp>

#include <Utility/Types.hpp>

#include <vector>

namespace NVMe
{
    struct LBA
    {
        u16 MetaSize;
        u8  DataSize;
        u8  RelativePerformance;
    } __attribute__((packed));
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
        u64 Unusued3[5];
        u8  GuID[16];
        u8  ExtendedUID[8];
        LBA LbaFormatUpper[16];
        u64 Unused3[24];
        u8  VendorSpecific[3712];
    } __attribute__((packed));

    struct CachedBlock
    {
        u8* Cache;
        u64 Block;
        u64 End;
        i32 Status;
    };

    class Queue;
    class NameSpace : public ::Device
    {
      public:
        NameSpace(u32 id, class Controller* controller);
        virtual ~NameSpace() = default;

        bool         Initialize();

        inline usize GetMaxPhysRPgs() const { return m_MaxPhysRPages; }

        virtual std::string_view GetName() const noexcept override;

        virtual isize Read(void* dest, off_t offset, usize bytes) override
        {
            return 0;
        }
        virtual isize Write(const void* src, off_t offset, usize bytes) override
        {
            return 0;
        }

        virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }

      private:
        u32                 m_ID            = 0;
        Controller*         m_Controller    = nullptr;
        usize               m_MaxPhysRPages = 0;
        std::vector<Queue*> m_IoQueues;

        u64                 m_LbaSize        = 0;
        u64                 m_LbaCount       = 0;
        u64                 m_CacheBlockSize = 0;
        stat                m_Stats;
        CachedBlock*        m_Cache = nullptr;

        bool                Identify(NameSpaceInfo* nsid);
    };
}; // namespace NVMe
