/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Storage/GPT.hpp>
#include <Drivers/Storage/PartitionTable.hpp>
#include <Drivers/Storage/StorageDevice.hpp>

#include <Prism/Utility/Checksum.hpp>

bool PartitionTable::Load(class StorageDevice& device)
{
    m_Device = &device;
    m_Mbr    = CreateScope<MasterBootRecord>();
    Assert(m_Mbr);
    device.Read(m_Mbr.Raw(), 0, sizeof(MasterBootRecord));

    if (ParseGPT()) return true;
    return ParseMBR();
}

inline constexpr u8 MBR_SYSTEMID_EBR = 0x05;

bool                PartitionTable::ParseGPT()
{
    MBREntry& firstEntry = m_Mbr->PartitionEntries[0];
    if (firstEntry.Status != 0 || firstEntry.Type != 0xee
        || firstEntry.FirstSector != 1)
        return false;

    usize                       headerSize = sizeof(GPT::PartitionHeader);
    Scope<GPT::PartitionHeader> gptHeader = CreateScope<GPT::PartitionHeader>();
    auto                        nread
        = TryOrRetVal(m_Device->Read(gptHeader.Raw(), 512, headerSize), false);
    if (static_cast<usize>(nread) != headerSize) return false;

    u64 headerSignature = *reinterpret_cast<u64*>(gptHeader->Signature);
    if (headerSignature != GPT::HEADER_SIGNATURE)
    {
        LogError("GPT: Invalid header signature => {:#x}", headerSignature);
        return false;
    }
    if (gptHeader->HeaderSize < GPT::HEADER_LENGTH)
    {
        LogError("GPT: Invalid header size => {:#x}, should be: {:#x}",
                 gptHeader->HeaderSize, GPT::HEADER_LENGTH);
        return false;
    }

    u32 oldCrc             = gptHeader->Crc32Header;
    gptHeader->Crc32Header = 0;
    u32 crc32 = CRC32::DoChecksum(reinterpret_cast<u8*>(gptHeader.Raw()), 92);
    gptHeader->Crc32Header = oldCrc;

    if (crc32 != oldCrc)
        LogError("GPT: Invalid crc32 header checksum, crc32: {}, old crc32: {}",
                 crc32, oldCrc);
    if (gptHeader->Reserved != 0)
        LogWarn("GPT: Header's reserved field is: {:#x}, should be 0",
                gptHeader->Reserved);
    if (gptHeader->CurrentLBA != 1)
        LogWarn("GPT: Invalid header lba: {:#x}", gptHeader->CurrentLBA);
    LogDebug("GPT: Backup header LBA: {:#x}", gptHeader->BackupLBA);
    if (gptHeader->FirstBlock > gptHeader->LastBlock)
    {
        LogError("GPT: Invalid table bounds: {:#x}-{:#x}",
                 gptHeader->FirstBlock, gptHeader->LastBlock);
    }

    usize partitionArraySize = gptHeader->PartitionCount * gptHeader->EntrySize;

    Scope<u8[]> partitionArray = new u8[partitionArraySize];
    nread = TryOrRetVal(m_Device->Read(partitionArray.Raw(),
                                       gptHeader->PartitionArrayLBA * 512,
                                       partitionArraySize),
                        false);
    if (static_cast<usize>(nread) != partitionArraySize)
        LogWarn(
            "GPT: Failed to read whole partition array into memory, loaded "
            "{:#x} bytes",
            nread);

    u32 partitionArrayCrc32
        = CRC32::DoChecksum(partitionArray.Raw(), partitionArraySize);
    if (partitionArrayCrc32 != gptHeader->PartitionArrayCRC32)
        LogError(
            "GPT: Invalid partition array crc32 checksum: computed => {:#x}, "
            "actual => {:#x}",
            partitionArrayCrc32, gptHeader->PartitionArrayCRC32);

    GPT::Entry entry{};
    for (usize offset = 0, index = 0; offset < partitionArraySize;
         offset += sizeof(GPT::Entry))
    {
        Memory::Copy(&entry, partitionArray.Raw() + offset, sizeof(entry));
        if (*reinterpret_cast<u32*>(entry.UniqueGUID) == 0
            && *reinterpret_cast<u32*>(&entry.UniqueGUID[8]) == 0)
            continue;
        if (entry.Attributes
            & (GPT::Attributes::eDontMount | GPT::Attributes::eLegacy))
            continue;

        u64   firstBlock = entry.StartLBA;
        u64   lastBlock  = entry.EndLBA;
        u64   attributes = ToUnderlying(entry.Attributes);
        Entry partition{};
        partition.FirstBlock = firstBlock;
        partition.LastBlock  = lastBlock;
        partition.Attributes = attributes;

        LogDebug(
            "GPT: Discovered partition[{}]: {{ .StartLBA: {}, .EndLBA: {}, "
            ".Attributes: {} }}",
            index++, firstBlock, lastBlock, attributes);
        m_Entries.PushBack(partition);
    }

    return true;
}
bool PartitionTable::ParseMBR()
{
    for (usize i = 0; i < 4; i++)
    {
        MBREntry& entry = m_Mbr->PartitionEntries[i];
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
