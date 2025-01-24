/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

namespace PCI
{
    enum class AccessMethod
    {
        eNone             = 0,
        eLegacyAddressing = 1,
        eMemoryAddressing = 2,
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
        eSybsystemID         = 0x2e,
        eCapabilitiesPointer = 0x34,
        eInterruptLine       = 0x3c,
        eInterruptPin        = 0x3d,
        eMinGrant            = 0x3e,
        eMaxLatenccy         = 0x3f,
    };

    struct DeviceAddress;
    class IoSpace
    {
      public:
        virtual u64 Read(const DeviceAddress& address, RegisterOffset reg,
                         u8 byteWidth)
            = 0;
        virtual void Write(const DeviceAddress& address, RegisterOffset reg,
                           u64 value, u8 byteWidth)
            = 0;
    };

    class MemoryIoSpace : public IoSpace
    {
      public:
        MemoryIoSpace(Pointer address)
            : m_Address(address)
        {
        }

        virtual u64  Read(const DeviceAddress& address, RegisterOffset reg,
                          u8 byteWidth) override;
        virtual void Write(const DeviceAddress& address, RegisterOffset reg,
                           u64 value, u8 byteWidth) override;

      private:
        Pointer m_Address;
    };
}; // namespace PCI
