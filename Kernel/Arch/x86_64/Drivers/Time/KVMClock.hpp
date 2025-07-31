/*
 * Created by v1tr10l7 on 29.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Time/ClockSource.hpp>

namespace KVM
{
    struct CTOS_PACKED ClockInfo
    {
        u32 Version;
        u32 Padding0;
        u64 TscTimestamp;
        u64 SystemTime;
        u32 TscToSystemMultiplier;
        i8  TscShift;
        u8  Flags;
        u8  Padding1[2];
    };

    class Clock final : public ClockSource
    {
      public:
        static ErrorOr<Clock*>    Create();

        virtual StringView        Name() const override;

        virtual ErrorOr<void>     Enable() override;
        virtual ErrorOr<void>     Disable() override;

        virtual ErrorOr<Timestep> Now() override;

      private:
        ClockInfo   m_Info;
        u64         m_Offset = 0;

        static bool Supported();
    };
}; // namespace KVM
