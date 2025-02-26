/*
 * Created by vitriol1744 on 21.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/x86_64/Drivers/Timers/HPETComparator.hpp>
#include <Firmware/ACPI/HPET.hpp>

#include <vector>

namespace HPET
{
    constexpr usize ABSOLUTE_MAXIMUM_COUNTER_TICK_PERIOD = 0x05f5e100;

    using Table                                          = ACPI::SDTs::HPET;
    class TimerBlock
    {
      public:
        static ErrorOr<TimerBlock*> GetFromTable(PM::Pointer hpetPhys);

        TimerBlock() = default;
        TimerBlock(Table* hpet);

        void            Enable() const;
        void            Disable() const;
        constexpr usize NsToRawCounterTicks(u64 ns) const
        {
            return (ns * 1000000ull) / (u64)m_MainCounterRegister;
        }

        constexpr usize RawCounterTicksToNs(u64 rawTicks) const
        {
            return (rawTicks * m_MainCounterRegister * 100ull)
                 / ABSOLUTE_MAXIMUM_COUNTER_TICK_PERIOD;
        }

        u64  GetCounterValue() const;
        void Sleep(u64 us) const;

      private:
        u16                         m_VendorID            = 0;
        u16                         m_MinimimumTick       = 0;
        usize                       m_TimerCount          = 0;
        bool                        m_X64Capable          = false;
        usize                       m_Frequency           = 0;
        usize                       m_MainCounterRegister = 0;
        struct Entry*               m_Entry               = nullptr;
        std::vector<HPETComparator> m_Comparators;
    };

    ErrorOr<void>                  DetectAndSetup();

    const std::vector<TimerBlock>& GetDevices();
}; // namespace HPET
