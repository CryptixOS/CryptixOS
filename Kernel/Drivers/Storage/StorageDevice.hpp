/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Device.hpp>
#include <Drivers/Storage/PartitionTable.hpp>

class StorageDevice : public Device
{
  public:
    StorageDevice(u16 major, u16 minor)
        : Device(DriverType(major), DeviceType(minor))
    {
    }

    void LoadPartitionTable();

  protected:
    PartitionTable m_PartitionTable;
};
