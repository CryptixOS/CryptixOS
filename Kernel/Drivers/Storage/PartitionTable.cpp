/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Storage/MasterBootRecord.hpp>
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

bool PartitionTable::ParseMBR(MasterBootRecord* mbr)
{
    for (usize i = 0; i < 4; i++)
    {
        MBREntry& entry = mbr->PartitionEntries[i];
        if (entry.FirstSector == 0x00) continue;

        u64 firstBlock = entry.FirstSector;
        u64 lastBlock  = firstBlock + entry.SectorCount - 1;
        u64 type       = entry.Type;

        LogInfo("Entry[{}]: {{ firstBlock: {}, lastBlock: {}, type: {} }}", i,
                firstBlock, lastBlock, type);
        m_Entries.push_back({firstBlock, lastBlock, type});
    }

    return false;
}
bool PartitionTable::ParseGPT() { return false; }
