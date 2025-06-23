/*
 * Created by vitriol1744 on 21.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/ACPI/HPET.hpp>
#include <Prism/Containers/Vector.hpp>

namespace HPET
{
    using Table = ACPI::SDTs::HPET;
    class TimerBlock
    {
      public:
        static ErrorOr<TimerBlock*> GetFromTable(Pointer hpetPhys);

        TimerBlock() = default;
        TimerBlock(Table* hpet);

        void Disable() const;

        u64  GetCounterValue() const;
        void Sleep(u64 us) const;

      private:
        struct Entry* m_Entry    = nullptr;
        usize         tickPeriod = 0;
    };

    ErrorOr<void>             DetectAndSetup();

    const Vector<TimerBlock>& GetDevices();
}; // namespace HPET
