/*
 * Created by v1tr10l7 on 14.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/termios.h>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Pointer.hpp>

export module Cryptix:Firmware.DeviceTree.DeviceTree;

export import :Firmware.DeviceTree.Node;

export constexpr usize FDT_MAGIC = 0xd00dfeed;

namespace DeviceTree
{
    export enum class FDT_TokenType : u32
    {
        eBeginNode = 0x01,
        eEndNode   = 0x02,
        eProperty  = 0x03,
        eNop       = 0x04,
        eEnd       = 0x09,
    };
    export struct FDT_Header
    {
        BigEndian<u32> Magic;
        BigEndian<u32> TotalSize;

        BigEndian<u32> StructureBlockOffset;
        BigEndian<u32> StringBlockOffset;
        BigEndian<u32> MemoryReservationBlockOffset;

        BigEndian<u32> Version;
        BigEndian<u32> LastCompatibleVersion;

        BigEndian<u32> BootCPUID;
        BigEndian<u32> StringBlockLength;
        BigEndian<u32> StructureBlockLength;
    };
    static_assert(sizeof(FDT_Header) == 40,
                  "FDT header's size doesn't match the specification!");

    export struct Driver
    {
        String     Name;
        StringView Compatible;

        using ProbeFn = ErrorOr<void> (*)(Node& node);

        ProbeFn Probe;
    };
    export bool Initialize(Pointer deviceTreeBlob);

    export bool RegisterDriver(const Driver& driver);
}; // namespace DeviceTree
