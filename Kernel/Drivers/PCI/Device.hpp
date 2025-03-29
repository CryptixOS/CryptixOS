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

#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Delegate.hpp>
#include <Prism/Spinlock.hpp>

#include <span>

namespace PCI
{

    struct Bar
    {
        Pointer   Base         = 0;
        Pointer   Address      = 0;
        usize     Size         = 0;

        bool      IsMMIO       = false;
        bool      DisableCache = false;
        bool      Is64Bit      = false;

        Pointer   Map(usize alignment);

        constexpr operator bool() const { return Address != nullptr; }
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
    struct [[gnu::packed]] MsiControl
    {
        u16 MsiE     : 1;
        u16 Mmc      : 3;
        u16 Mme      : 3;
        u16 C64      : 1;
        u16 PVM      : 1;
        u16 Reserved : 6;
    };
    struct [[gnu::packed]] MsiAddress
    {
        u32 Reserved0       : 2;
        u32 DestinationMode : 1;
        u32 RedirectionHint : 1;
        u32 Reserved1       : 8;
        u32 DestinationID   : 8;
        u32 BaseAddress     : 12;
    };
    struct [[gnu::packed]] MsiData
    {
        u32 Vector       : 8;
        u32 DeliveryMode : 3;
        u32 Reserved0    : 3;
        u32 Level        : 1;
        u32 TriggerMode  : 1;
        u32 Reserved1    : 16;
    };

    struct [[gnu::packed]] MsiXControl
    {
        u16 Irqs     : 11;
        u16 Reserved : 3;
        u16 Mask     : 1;
        u16 Enable   : 1;
    };
    struct [[gnu::packed]] MsiXAddress
    {
        u32 Bir    : 3;
        u32 Offset : 29;
    };
    struct [[gnu::packed]] MsiXVectorCtrl
    {
        u32 Mask     : 8;
        u32 Reserved : 31;
    };
    struct [[gnu::packed]] MsiXEntry
    {
        u32 AddressLow;
        u32 AddressHigh;
        u32 Data;
        u32 Control;
    };
    class Device
    {
      public:
        Device(const DeviceAddress& address)
            : m_Address(address)
        {
            m_ID.VendorID    = GetVendorID();
            m_ID.ID          = Read<u16>(RegisterOffset::eDeviceID);
            m_ID.SubsystemID = Read<u16>(RegisterOffset::eSubsystemID);
            m_ID.SubsystemVendorID
                = Read<u16>(RegisterOffset::eSubsystemVendorID);
            m_ID.Class    = Read<u8>(RegisterOffset::eClassID);
            m_ID.Subclass = Read<u8>(RegisterOffset::eSubClassID);

            auto capabilitiesPointer
                = Read<u8>(RegisterOffset::eCapabilitiesPointer);
            while (capabilitiesPointer)
            {
                u16 header       = ReadAt(capabilitiesPointer, 2);
                u8  capabilityID = header & 0xff;
                switch (capabilityID)
                {
                    case 0x05:
                        m_MsiSupported = true;
                        m_MsiOffset    = capabilitiesPointer;
                        break;
                    case 0x11:
                        m_MsixSupported = true;
                        m_MsixOffset    = capabilitiesPointer;
                        MsiXControl control(
                            ReadAt(capabilitiesPointer + 0x02, 2));
                        MsiXAddress table(
                            ReadAt(capabilitiesPointer + 0x04, 4));
                        MsiXAddress pending(
                            ReadAt(capabilitiesPointer + 0x08, 4));

                        usize count    = control.Irqs;
                        m_MsixMessages = count;
                        m_MsixIrqs.Allocate(count);

                        m_MsixTableBar      = table.Bir;
                        m_MsixTableOffset   = table.Offset << 3;

                        m_MsixPendingBar    = pending.Bir;
                        m_MsixPendingOffset = pending.Offset << 3;
                        break;
                }

                capabilitiesPointer = (header >> 8) & 0xfc;
            }
        }

        void                        EnableMemorySpace();
        void                        EnableBusMastering();

        constexpr const DeviceID&   GetDeviceID() const { return m_ID; }
        inline const DeviceAddress& GetAddress() const { return m_Address; }
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
        Bar  GetBar(u8 index);

        bool RegisterIrq(u64 cpuid, Delegate<void()> handler);

      protected:
        Spinlock         m_Lock;
        DeviceAddress    m_Address{};
        DeviceID         m_ID;
        bool             m_MsiSupported  = false;
        u8               m_MsiOffset     = 0;
        bool             m_MsixSupported = false;
        u8               m_MsixOffset    = 0;
        u16              m_MsixMessages  = 0;
        Bitmap           m_MsixIrqs;
        u8               m_MsixTableBar;
        u32              m_MsixTableOffset;
        u8               m_MsixPendingBar;
        u32              m_MsixPendingOffset;
        Delegate<void()> m_OnIrq;

        bool             MsiSet(u64 cpuid, u16 vector, u16 index);
        bool             MsiXSet(u64 cpuid, u16 vector, u16 index);

        void             SendCommand(u16 cmd, bool flag)
        {
            u16 command = Read<u16>(RegisterOffset::eCommand);

            if (flag) command |= cmd;
            else command &= ~cmd;
            Write<u16>(RegisterOffset::eCommand, command);
        }

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
