/*
 * Created by vitriol1744 on 21.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <vector>

namespace HPET
{
    class Device
    {
      public:
        Device() = default;
        Device(struct Table* hpet);

        void Disable() const;

        u64  GetCounterValue() const;
        void Sleep(u64 us) const;

      private:
        struct Entry* entry      = nullptr;
        usize         tickPeriod = 0;
    };

    void                       Initialize();

    const std::vector<Device>& GetDevices();
}; // namespace HPET
