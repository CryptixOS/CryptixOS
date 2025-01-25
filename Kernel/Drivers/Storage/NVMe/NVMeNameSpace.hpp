/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

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

    class NameSpace
    {
      public:
        NameSpace(u32 id, class Controller* controller);

        bool Initialize();

      private:
        u32         m_ID            = 0;
        Controller* m_Controller    = nullptr;
        usize       m_MaxPhysRPages = 0;

        bool        Identify(NameSpaceInfo* nsid);
    };
}; // namespace NVMe
