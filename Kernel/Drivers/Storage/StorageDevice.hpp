/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/BlockDevice.hpp>
#include <Drivers/Storage/PartitionTable.hpp>

class StorageDevice : public BlockDevice
{
  public:
    StorageDevice(StringView name, DeviceID id)
        : BlockDevice(name, id)
    {
    }

    void LoadPartitionTable();

  protected:
    PartitionTable m_PartitionTable;
};
