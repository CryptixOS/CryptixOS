/*
 * Created by v1tr10l7 on 28.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Storage/StorageDevice.hpp>

class StorageDevicePartition : public Device
{
  public:
    StorageDevicePartition(StorageDevice& device, u64 firstBlock, u64 lastBlock,
                           u16 majorID, u16 minorID)
        : Device(majorID, minorID)
        , m_Device(device)
        , m_FirstBlock(firstBlock)
        , m_LastBlock(lastBlock)
    {
    }

    virtual StringView GetName() const noexcept override
    {
        return m_Device.GetName();
    }

    virtual isize Read(void* dest, off_t offset, usize bytes) override
    {
        // if (m_FirstBlock + offset >= m_LastBlock) return_err(-1, ENODEV);
        return m_Device.Read(
            dest, m_Device.GetStats().st_blksize * m_FirstBlock + offset,
            bytes);
    }
    virtual isize Write(const void* src, off_t offset, usize bytes) override
    {
        if (m_FirstBlock + offset >= m_LastBlock) return_err(-1, ENODEV);
        return m_Device.Write(src, m_FirstBlock + offset, bytes);
    }

    virtual i32 IoCtl(usize request, uintptr_t argp) override
    {
        return m_Device.IoCtl(request, argp);
    }

  private:
    StorageDevice& m_Device;
    u64            m_FirstBlock;
    u64            m_LastBlock;
};
