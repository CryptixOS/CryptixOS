/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Device.hpp>
#include <Drivers/PCI/Access.hpp>
#include <Drivers/PCI/Definitions.hpp>

#include <Library/Spinlock.hpp>
#include <Prism/Types.hpp>

namespace PCI
{

    struct Bar
    {
        PM::Pointer Address      = 0;
        usize       Size         = 0;

        bool        IsMMIO       = false;
        bool        DisableCache = false;

        constexpr   operator bool() const { return Address != nullptr; }
    };

    struct DeviceAddress
    {
        u32  Domain                                  = 0;
        u8   Bus                                     = 0;
        u8   Slot                                    = 0;
        u8   Function                                = 0;

        auto operator<=>(const DeviceAddress&) const = default;
    };
    struct DeviceID
    {
        u16 VendorID = 0;
        u16 ID       = 0;

        operator bool() const { return !VendorID && !ID; }
        auto operator<=>(const DeviceID&) const = default;
    };

    struct Domain
    {
        Domain() = delete;
        Domain(u32 id, u8 busStart, u8 busEnd)
            : ID(id)
            , BusStart(busStart)
            , BusEnd(busEnd)
        {
        }

        u32 ID;
        u8  BusStart;
        u8  BusEnd;
    };
    class Device : public ::Device
    {
      public:
        Device(const DeviceAddress& address, DriverType major, DeviceType minor)
            : ::Device(major, minor)
            , m_Address(address)
        {
        }

        void          EnableMemorySpace();
        void          EnableBusMastering();

        constexpr u16 GetVendorID() const
        {
            return Read<u16>(RegisterOffset::eVendorID);
        }
        constexpr u16 GetDeviceID() const
        {
            return Read<u16>(RegisterOffset::eDeviceID);
        }

        usize              GetInterruptLine();

        inline PM::Pointer GetBarAddress(u8 index) const
        {
            u32 offset = index * 4 + std::to_underlying(RegisterOffset::eBar0);
            return Read<u16>(static_cast<RegisterOffset>(offset));
        }
        Bar GetBar(u8 index);

      protected:
        Spinlock      m_Lock;
        DeviceAddress m_Address{};

        template <std::unsigned_integral T>
            requires(sizeof(T) <= 4)
        T Read(RegisterOffset offset) const
        {
            return ReadAt(std::to_underlying(offset), sizeof(T));
        }
        template <std::unsigned_integral T>
        void Write(RegisterOffset offset, T value) const
            requires(sizeof(T) <= 4)
        {
            WriteAt(std::to_underlying(offset), value, sizeof(T));
        }

        u32  ReadAt(u32 offset, i32 accessSize) const;
        void WriteAt(u32 offset, u32 value, i32 accessSize) const;
    };
}; // namespace PCI
