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
#include <Drivers/PCI/MSI.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/Delegate.hpp>

namespace PCI
{
    using Register = RegisterOffset;

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

    struct Capability
    {
        CapabilityID ID     = CapabilityID::eNull;
        u64          Offset = 0;

        Capability()        = default;
        Capability(CapabilityID id, u64 offset)
            : ID(id)
            , Offset(offset)
        {
        }
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

    enum class PowerState
    {
        eD0,
        eD1,
        eD2,
        eD3,
    };
    class Device
    {
      public:
        Device(const DeviceAddress& address);

        ErrorOr<void>               Enable();

        void                        EnableIOSpace();
        void                        DisableIOSpace();

        void                        EnableMemorySpace();
        void                        DisableMemorySpace();

        void                        EnableBusMastering();
        void                        DisableBusMastering();

        void                        EnableInterrupts();
        void                        DisableInterrupts();

        enum PowerState             PowerState() const;
        ErrorOr<void>               SetPowerState(enum PowerState state);

        Optional<Capability>        FindCapability(CapabilityID id);

        constexpr const DeviceID&   GetDeviceID() const { return m_ID; }
        inline const DeviceAddress& GetAddress() const { return m_Address; }
        bool          MatchID(Span<DeviceID> idTable, DeviceID& outID);

        constexpr u16 GetVendorID() const
        {
            return Read<u16>(RegisterOffset::eVendorID);
        }

        usize          GetInterruptLine();

        inline Pointer GetBarAddress(u8 index) const
        {
            u32 offset = index * 4 + ToUnderlying(RegisterOffset::eBar0);
            return Read<u16>(static_cast<RegisterOffset>(offset));
        }
        Bar  GetBar(u8 index);

        bool RegisterIrq(u64 cpuid, Delegate<void()> handler);

      protected:
        friend struct Capability;

        Spinlock           m_Lock;
        DeviceAddress      m_Address{};
        DeviceID           m_ID;

        Vector<Capability> m_Capabilities;
        bool               m_MsiSupported  = false;
        u8                 m_MsiOffset     = 0;
        bool               m_MsixSupported = false;
        u8                 m_MsixOffset    = 0;
        u16                m_MsixMessages  = 0;
        Bitmap             m_MsixIrqs;
        u8                 m_MsixTableBar;
        u32                m_MsixTableOffset;
        u8                 m_MsixPendingBar;
        u32                m_MsixPendingOffset;
        Delegate<void()>   m_OnIrq;

        bool               MsiSet(u64 cpuid, u16 vector, u16 index);
        bool               MsiXSet(u64 cpuid, u16 vector, u16 index);

        void               SendCommand(Command cmd, bool flag)
        {
            u16 command = Read<u16>(RegisterOffset::eCommand);

            if (flag) command |= ToUnderlying(cmd);
            else command &= ~ToUnderlying(cmd);
            Write<u16>(RegisterOffset::eCommand, command);
        }

        template <UnsignedIntegral T>
            requires(sizeof(T) <= 4)
        T Read(RegisterOffset offset) const
        {
            return ReadAt(ToUnderlying(offset), sizeof(T));
        }
        template <UnsignedIntegral T>
        void Write(RegisterOffset offset, T value) const
            requires(sizeof(T) <= 4)
        {
            WriteAt(ToUnderlying(offset), value, sizeof(T));
        }

        u32  ReadAt(u32 offset, i32 accessSize) const;
        void WriteAt(u32 offset, u32 value, i32 accessSize) const;

      private:
        void ReadCapabilities();
    };
}; // namespace PCI
