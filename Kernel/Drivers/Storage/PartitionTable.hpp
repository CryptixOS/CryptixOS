/*
 * Created by v1tr10l7 on 26.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Storage/MasterBootRecord.hpp>

#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>

class PartitionTable
{
  public:
    bool Load(class StorageDevice& device);

    struct Entry
    {
        Entry() = default;
        Entry(u64 firstBlock, u64 lastBlock, u64 type)
            : FirstBlock(firstBlock)
            , LastBlock(lastBlock)
            , Type(type)
        {
        }

        u64 FirstBlock;
        u64 LastBlock;
        u64 Type;
        u64 UUID;
        u64 Attributes;
    };

    auto begin() { return m_Entries.begin(); }
    auto end() { return m_Entries.end(); }

  private:
    StorageDevice* m_Device;
    Vector<Entry>  m_Entries;

    bool           IsProtective(struct MasterBootRecord* mbr);

    bool           ParseMBR(MasterBootRecord* mbr);
    bool           ParseEBR(MasterBootRecord& ebr, usize offset = 0);
    bool           ParseGPT();
};
