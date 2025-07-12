/*
 * Created by v1tr10l7 on 12.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/UUID.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Network/Ipv4Address.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Utility/PathView.hpp>

enum class BootMediaType
{
    eGeneric = 0,
    eOptical = 1,
    eTFTP    = 2,
};
struct BootModuleInfo
{
    StringView    Name              = ""_sv;
    PathView      Path              = ""_sv;

    // Virtual address to where the module was loaded
    Pointer       LoadAddress       = nullptr;
    usize         Size              = 0;

    BootMediaType MediaType         = BootMediaType::eGeneric;
    IPv4Address   TFTP_IPv4         = "127.0.0.1"_sv;
    u32           TFTP_Port         = 21;

    usize         PartitionIndex    = 0;
    usize         MBR_DiskID        = 0;
    UUID          GPT_DiskUUID      = 0;
    UUID          GPT_PartitionUUID = 0;
    UUID          FilesystemUUID    = 0;
};
