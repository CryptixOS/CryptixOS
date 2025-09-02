/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>
#include <Drivers/Input/InputDevice.hpp>

#include <Prism/Core/Error.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Ref.hpp>

class SerioController : public CharacterDevice
{
  public:
    enum class DevicePort
    {
        eNone  = 0x00,
        ePort1 = 0x01,
        ePort2 = 0x02,
    };

    enum class DeviceCommand
    {
        eIdentify        = 0xf2,
        eEnableScanning  = 0xf4,
        eDisableScanning = 0xf5,
        eReset           = 0xff,
    };

    static ErrorOr<void>          Register(::Ref<SerioController> ctrl);
    static ::Ref<SerioController> Instance();

    virtual ~SerioController()                           = default;

    virtual bool          IsOutputEmpty()                = 0;

    virtual ErrorOr<void> EnableDevice(DevicePort port)  = 0;
    virtual ErrorOr<void> DisableDevice(DevicePort port) = 0;

    virtual ErrorOr<void> EnablePort1Translation()       = 0;
    virtual ErrorOr<void> DisablePort1Translation()      = 0;

    virtual ErrorOr<void> ResetDevice(DevicePort port)   = 0;

    virtual ErrorOr<void> SendDeviceCommand(DevicePort    port,
                                            DeviceCommand command)
        = 0;
    virtual ErrorOr<void> SendDeviceCommand(DevicePort    port,
                                            DeviceCommand command, u8 data)
        = 0;

  protected:
    static SerioController* s_Instance;

    SerioController(StringView name, DeviceMajor major, DeviceMinor minor)
        : CharacterDevice(name, major, minor)
    {
    }
};
using SerioDevicePort = SerioController::DevicePort;

class SerioDevice : public InputDevice
{
  public:
    inline SerioDevice(StringView name, SerioController* serio)
        : InputDevice(name, InputDevice::AllocateMinor())
    {
    }
    virtual void OnByteReceived(u8 byte) = 0;
};
