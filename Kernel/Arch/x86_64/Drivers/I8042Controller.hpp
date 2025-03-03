/*
 * Created by v1tr10l7 on 05.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/x86_64/Types.hpp>

#include <errno.h>
#include <expected>
#include <utility>

namespace I8042Controller
{
    enum class Port : u16
    {
        eBuffer  = 0x60,
        eCommand = 0x64,
        eStatus  = 0x64,
    };
    enum class Status : u8
    {
        // Data sent from ps/2 controller
        eOutBufferFull = Bit(0),
        // Data to be read by ps/2 controller
        eInBufferFull  = Bit(1),
        eSystem        = Bit(2),
        // if 0, then data written to input buffer is data for ps/2 device,
        // otherwise it's a command for ps/2 controller
        eCommand       = Bit(3),
        eTimeOutError  = Bit(6),
        eParityError   = Bit(7),
    };
    enum class Command : u8
    {
        eReadConfigurationByte     = 0x20,
        eWriteConfigurationByte    = 0x60,
        eDisablePort2              = 0xa7,
        eEnablePort2               = 0xa8,
        eTestPort2                 = 0xa9,
        eTestController            = 0xaa,
        eTestPort1                 = 0xab,
        eDiagnosticDump            = 0xac,
        eDisablePort1              = 0xad,
        eEnablePort1               = 0xae,
        eReadControllerInput       = 0xc0,
        // Copy bits (0-3) of input port to status bits(4-7)
        eCopyInputLow2StatusHigh   = 0xc1,
        // Copy bits (4-7) of input port to status bits(4-7)
        eCopyInputHigh2StatusHigh  = 0xc1,
        eReadControllerOutput      = 0xd0,
        eWriteNextControllerOutput = 0xd1,
        eWriteNextOutPort1         = 0xd2,
        eWriteNextOutPort2         = 0xd3,
        eWriteToPort2              = 0xd4,
        eResetCPU                  = 0xfe,
    };
    enum class Configuration : u8
    {
        eEnablePort1Irq         = Bit(0),
        eEnablePort2Irq         = Bit(1),
        eSystemPassedPost       = Bit(2),
        eDisablePort1Clock      = Bit(4),
        eDisablePort2Clock      = Bit(5),
        eEnablePort1Translation = Bit(6),
    };
    inline u8 operator~(Configuration lhs) { return ~std::to_underlying(lhs); }
    inline u8 operator&(u8 lhs, const Configuration rhs)
    {
        return lhs & std::to_underlying(rhs);
    }
    inline Configuration operator|(Configuration lhs, const Configuration rhs)
    {
        u8 config = std::to_underlying(lhs) | std::to_underlying(rhs);

        return static_cast<Configuration>(config);
    }
    inline u8& operator&=(u8& lhs, const Configuration rhs)
    {
        lhs &= std::to_underlying(rhs);

        return lhs;
    }
    inline u8& operator|=(u8& lhs, const Configuration rhs)
    {
        lhs |= std::to_underlying(rhs);

        return lhs;
    }

    enum class Output
    {
        eSystemReset = Bit(0),
        eA20Gate     = Bit(1),
        ePort2Clock  = Bit(2),
        ePort2Data   = Bit(3),
        ePort1Clock  = Bit(6),
        ePort1Data   = Bit(7),
    };

    enum Response
    {
        ePortTestSuccess        = 0x00,
        ePortClockLineStuckLow  = 0x01,
        ePortClockLineStuckHigh = 0x02,
        ePortDataLineStuckLow   = 0x03,
        ePortDataLineStuckHigh  = 0x04,

        eSelfTestSuccess        = 0x55,
        eDeviceTestPassed       = 0xaa,
        eAcknowledge            = 0xfa,
        eResend                 = 0xfe,
    };

    enum class DeviceCommand
    {
        eIdentify        = 0xf2,
        eEnableScanning  = 0xf4,
        eDisableScanning = 0xf5,
        eReset           = 0xff,
    };
    enum class DevicePort
    {
        eNone  = 0,
        ePort1 = 1,
        ePort2 = 2,
    };

    // TODO(v1tr10l7): Detect the ps/2 controller, instead of assuming it exists
    void                         Initialize();

    bool                         IsOutputEmpty();
    bool                         IsInputEmpty();

    u8                           ReadBlocking();
    void                         WriteBlocking(Port port, u8 data);

    std::expected<u8, errno_t>   TryRead();
    std::expected<void, errno_t> TryWrite(Port port, u8 data);

    void                         Flush();

    inline void                  SendCommand(Command command)
    {
        WriteBlocking(Port::eCommand, std::to_underlying(command));
    }

    std::expected<void, errno_t> WriteToDevice(DevicePort port, byte data);

    inline std::expected<void, errno_t> SendDeviceCommand(DevicePort    port,
                                                          DeviceCommand command)
    {
        return WriteToDevice(port, std::to_underlying(command));
    }

    std::expected<void, errno_t> ResetDevice(DevicePort port);

}; // namespace I8042Controller

using I8042Port          = I8042Controller::Port;
using I8042Command       = I8042Controller::Command;
using I8042Configuration = I8042Controller::Configuration;
using I8042Response      = I8042Controller::Response;
using PS2Port            = I8042Controller::DevicePort;
