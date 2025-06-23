/*
 * Created by v1tr10l7 on 23.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Storage/StorageDevicePartition.hpp>

StorageDevicePartition::StorageDevicePartition(StorageDevice& device,
                                               u64 firstBlock, u64 lastBlock,
                                               u16 majorID, u16 minorID)

    : Device(majorID, minorID)
    , m_Device(device)
    , m_FirstBlock(firstBlock)
    , m_LastBlock(lastBlock)
{
    m_Stats.st_blksize = m_Device.GetStats().st_blksize;
    m_Stats.st_size    = lastBlock - firstBlock;
    m_Stats.st_blocks  = m_Stats.st_size / m_Stats.st_blksize;
    m_Stats.st_rdev    = 0;
    m_Stats.st_mode    = 0666 | S_IFBLK;
}
