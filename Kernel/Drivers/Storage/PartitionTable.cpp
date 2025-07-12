/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Storage/PartitionTable.hpp>
#include <Drivers/Storage/StorageDevice.hpp>

bool PartitionTable::Load(class StorageDevice& device)
{
    m_Device              = &device;
    MasterBootRecord* mbr = new MasterBootRecord;
    device.Read(mbr, 0, sizeof(MasterBootRecord));

    bool loaded = false;
    if (IsProtective(mbr)) loaded = ParseGPT();
    if (!loaded) loaded = ParseMBR(mbr);

    delete mbr;
    return loaded;
}

bool PartitionTable::IsProtective(MasterBootRecord* mbr)
{
    // TODO(v1tr10l7): Check whether it is protective mbr
    MBREntry& firstEntry = mbr->PartitionEntries[0];
    if (firstEntry.Status != 0 || firstEntry.Type != 0xee
        || firstEntry.FirstSector != 1)
        return false;

    return false;
}

inline constexpr u8 MBR_SYSTEMID_EBR = 0x05;
bool                PartitionTable::ParseMBR(MasterBootRecord* mbr)
{
    for (usize i = 0; i < 4; i++)
    {
        MBREntry& entry = mbr->PartitionEntries[i];
        if (entry.FirstSector == 0x00) continue;

        u64 firstBlock = entry.FirstSector;
        u64 lastBlock  = firstBlock + entry.SectorCount - 1;
        u8  type       = entry.Type;
        if (entry.Type == MBR_SYSTEMID_EBR)
        {
            LogInfo("PartitionTable: Discovered extended partitiond entry");
            MasterBootRecord* ebr = new MasterBootRecord;
            m_Device->Read(ebr, entry.FirstSector * 512,
                           sizeof(MasterBootRecord));

            ParseEBR(*ebr, entry.FirstSector);
            delete ebr;
            continue;
        }

        m_Entries.PushBack({firstBlock, lastBlock, type});
    }

    return false;
}
bool PartitionTable::ParseEBR(MasterBootRecord& ebr, usize offset)
{
    //
    auto logicalPartition = ebr.PartitionEntries[0];
    auto nextEbrEntry     = ebr.PartitionEntries[1];

    u64  firstBlock       = logicalPartition.FirstSector + offset;
    u64  lastBlock        = firstBlock + logicalPartition.SectorCount - 1;
    u8   type             = logicalPartition.Type;
    m_Entries.EmplaceBack(firstBlock, lastBlock, type);

    type = nextEbrEntry.Type;
    if (type == MBR_SYSTEMID_EBR) return false;

    offset += nextEbrEntry.FirstSector;
    auto nextEbr = new MasterBootRecord;
    m_Device->Read(nextEbr, offset * 512, sizeof(MasterBootRecord));

    ParseEBR(*nextEbr, offset);

    delete nextEbr;
    return true;
}
bool PartitionTable::ParseGPT() { return false; }
