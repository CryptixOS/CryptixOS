/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

struct SDTHeader
{
    char signature[4];
    u32  length;
    u8   revision;
    u8   checksum;
    char oemID[6];
    u64  oemTableID;
    u32  oemRevision;
    u32  creatorID;
    u32  creatorRevision;
} __attribute__((packed));

namespace ACPI
{
    void       Initialize();
    SDTHeader* GetTable(const char* signature, usize index = 0);
    template <typename T>
    inline T* GetTable(const char* signature, usize index = 0)
    {
        return reinterpret_cast<T*>(GetTable(signature, index));
    }
} // namespace ACPI
