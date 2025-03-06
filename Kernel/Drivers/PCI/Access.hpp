/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

#include <unordered_map>

namespace PCI
{
    enum class AddressingMethod
    {
        eNone   = 0,
        eLegacy = 1,
        eMemory = 2,
    };
    enum class RegisterOffset
    {
        eVendorID            = 0x00,
        eDeviceID            = 0x02,
        eCommand             = 0x04,
        eStatus              = 0x06,
        eRevisionID          = 0x08,
        eProgIf              = 0x09,
        eSubClassID          = 0x0a,
        eClassID             = 0x0b,
        eCacheLineSize       = 0x0c,
        eLatencyTimer        = 0x0d,
        eHeaderType          = 0x0e,
        eBist                = 0x0f,
        eBar0                = 0x10,
        eBar1                = 0x14,
        eBar2                = 0x18,
        eSecondaryBus        = 0x19,
        ePrimaryBus          = 0x1a,
        eBar3                = 0x1c,
        eBar4                = 0x20,
        eBar5                = 0x24,
        eCardBusCisPointer   = 0x28,
        eSubsystemVendorID   = 0x2c,
        eSubsystemID         = 0x2e,
        eCapabilitiesPointer = 0x34,
        eInterruptLine       = 0x3c,
        eInterruptPin        = 0x3d,
        eMinGrant            = 0x3e,
        eMaxLatenccy         = 0x3f,
    };

    struct DeviceAddress;
    class AccessMechanism
    {
      public:
        virtual u32 Read(const DeviceAddress& address, u32 reg, i32 accessSize)
            = 0;
        virtual void Write(const DeviceAddress& address, u32 reg, u32 value,
                           i32 accessSize)
            = 0;
    };

    class LegacyAccessMechanism : public AccessMechanism
    {
      public:
#ifdef CTOS_TARGET_X86_64
        // NOTE(v1tr10l7): these are defined in Arch/${arch}/PCI.cpp
        virtual u32  Read(const DeviceAddress& address, u32 reg,
                          i32 accessSize) override;
        virtual void Write(const DeviceAddress& address, u32 reg, u32 value,
                           i32 accessSize) override;
#else
        LegacyAccessMechanism() { AssertNotReached(); }

        virtual u32 Read(const DeviceAddress& address, u32 reg,
                         i32 accessSize) override
        {
            CtosUnused(address);
            CtosUnused(reg);
            CtosUnused(accessSize);

            AssertNotReached();
            return 0;
        }
        virtual void Write(const DeviceAddress& address, u32 reg, u32 value,
                           i32 accessSize) override
        {
            CtosUnused(address);
            CtosUnused(reg);
            CtosUnused(accessSize);
            CtosUnused(value);

            AssertNotReached();
        }
#endif
    };
    class ECAM : public AccessMechanism
    {
      public:
        ECAM(PM::Pointer address, u8 busStart)
            : m_Base(address)
            , m_BusStart(busStart)
        {
        }

        virtual u32  Read(const DeviceAddress& address, u32 reg,
                          i32 accessSize) override;
        virtual void Write(const DeviceAddress& address, u32 reg, u32 value,
                           i32 accessSize) override;

      private:
        PM::Pointer                              m_Base;
        u8                                       m_BusStart;

        std::unordered_map<uintptr_t, uintptr_t> m_Mappings;

        uintptr_t GetAddress(const DeviceAddress& address, u32 offset);
    };
}; // namespace PCI
