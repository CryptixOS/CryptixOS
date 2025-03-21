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

#include <Prism/Core/Types.hpp>
#include <Prism/Delegate.hpp>
#include <Prism/Spinlock.hpp>

#include <span>

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
        constexpr static isize ANY_ID            = -1;

        i32                    VendorID          = 0;
        i32                    ID                = 0;
        i32                    SubsystemID       = 0;
        i32                    SubsystemVendorID = 0;
        i32                    Class             = 0;
        i32                    Subclass          = 0;

        DeviceID()                               = default;
        DeviceID(i32 vendorID, i32 id, i32 subsystemID, i32 subsystemVendorID,
                 i32 classID, i32 subclassID)
            : VendorID(vendorID)
            , ID(id)
            , SubsystemID(subsystemID)
            , SubsystemVendorID(subsystemVendorID)
            , Class(classID)
            , Subclass(subclassID)
        {
        }

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
        Device(const DeviceAddress& address, DriverType major = DriverType(0),
               DeviceType minor = DeviceType(0))
            : ::Device(major, minor)
            , m_Address(address)
        {
            m_ID.VendorID    = GetVendorID();
            m_ID.ID          = Read<u16>(RegisterOffset::eDeviceID);
            m_ID.SubsystemID = Read<u16>(RegisterOffset::eSubsystemID);
            m_ID.SubsystemVendorID
                = Read<u16>(RegisterOffset::eSubsystemVendorID);
            m_ID.Class    = Read<u8>(RegisterOffset::eClassID);
            m_ID.Subclass = Read<u8>(RegisterOffset::eSubClassID);
        }

        void                      EnableMemorySpace();
        void                      EnableBusMastering();

        constexpr const DeviceID& GetDeviceID() const { return m_ID; }
        bool          MatchID(std::span<DeviceID> idTable, DeviceID& outID);

        constexpr u16 GetVendorID() const
        {
            return Read<u16>(RegisterOffset::eVendorID);
        }

        usize              GetInterruptLine();

        inline PM::Pointer GetBarAddress(u8 index) const
        {
            u32 offset = index * 4 + std::to_underlying(RegisterOffset::eBar0);
            return Read<u16>(static_cast<RegisterOffset>(offset));
        }
        Bar                      GetBar(u8 index);

        virtual std::string_view GetName() const noexcept override
        {
            return "No Device";
        }

        virtual isize Read(void* dest, off_t offset, usize bytes) override
        {
            return -1;
        }
        virtual isize Write(const void* src, off_t offset, usize bytes) override
        {
            return -1;
        }
        virtual i32 IoCtl(usize request, uintptr_t argp) override { return -1; }

        bool        RegisterIrq(u64 cpuid, Delegate<void()> handler);

      protected:
        Spinlock      m_Lock;
        DeviceAddress m_Address{};
        DeviceID      m_ID;

        void          OnIrq(CPUContext* ctx);

        bool          MsiSet(u64 cpuid, u16 vector, u16 index);
        bool          MsiXSet(u64 cpuid, u16 vector, u16 index);

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
    struct Driver
    {
        std::string         Name;
        std::span<DeviceID> MatchIDs;

        using ProbeFn
            = ErrorOr<void>   (*)(DeviceAddress& address, const DeviceID& id);
        using RemoveFn = void (*)(Device& device);

        ProbeFn               Probe;
        RemoveFn              Remove;
    };
}; // namespace PCI
