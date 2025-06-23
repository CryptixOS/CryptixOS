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
                           u16 majorID, u16 minorID);

    virtual StringView GetName() const noexcept override
    {
        return m_Device.GetName();
    }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        // if (m_FirstBlock + offset >= m_LastBlock) return_err(-1, ENODEV);
        return m_Device.Read(
            dest, m_Device.GetStats().st_blksize * m_FirstBlock + offset,
            bytes);
    }
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override
    {
        // if (m_FirstBlock + offset >= m_LastBlock) return_err(-1, ENODEV);
        return m_Device.Write(
            src, m_Device.GetStats().st_blksize * m_FirstBlock + offset, bytes);
    }

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        return Read(out.Raw(), offset, count);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        return Write(in.Raw(), offset, count);
    }

    virtual i32 IoCtl(usize request, uintptr_t argp) override
    {
        return m_Device.IoCtl(request, argp);
    }

  private:
    StorageDevice&  m_Device;
    u64             m_FirstBlock;
    CTOS_UNUSED u64 m_LastBlock;
};
