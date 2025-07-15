/*
 * Created by v1tr10l7 on 15.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

enum class VideoInputType : u8
{
    eSeperateSync  = Bit(0),
    eCompositeSync = Bit(1),
    eSyncOnGreen   = Bit(2),
    eUnused        = Bit(3) | Bit(4),
    eVoltageLevel  = Bit(5) | Bit(6),
    eDigitalSygnal = Bit(7),
};
enum class DPMSFlags : u8
{
    eUnused             = Bit(0) | Bit(1),
    eDisplayTypeRGB     = Bit(3),
    eUnused2            = Bit(4),
    eActiveOffSupported = Bit(5),
    eSuspendSupported   = Bit(6),
    eStandbySupported   = Bit(7),
};
struct [[gnu::packed]] EDID
{
    u64                 Padding;
    u16                 ManufacturerID;
    u16                 ProductCode;
    u32                 Serial;
    u8                  ManufactureWeek;
    u8                  ManufactureYear;
    u8                  EDID_Version;
    u8                  EDID_Revision;
    enum VideoInputType VideoInputType;
    u8                  MaxHorizontalSize;
    u8                  MaxVerticalSize;
    u8                  GamaFactor;
    u8                  DPMSFlags;
    u8                  ChromaInformation[10];
    u8                  EstablishedTimings1;
    u8                  EstablishedTimings2;
    u8                  ManufactureReservedTimings;
    u16                 StandardTimingIdentification[8];
    u8                  DetailedTimingDescription1[18];
    u8                  DetailedTimingDescription2[18];
    u8                  DetailedTimingDescription3[18];
    u8                  DetailedTimingDescription4[18];
    u8                  Unused;
    u8                  Checksum;
};

static_assert(sizeof(EDID) == 0x80);
