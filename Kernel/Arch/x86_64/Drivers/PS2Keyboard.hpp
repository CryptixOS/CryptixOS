/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/x86_64/Drivers/I8042Controller.hpp>

#include <Drivers/Device.hpp>

enum class KeyModifiers
{
    eLeftShift,
    eRightShift,
    eLeftSuper,
    eRightSuper,
};

struct CPUContext;
class PS2Keyboard : public Device
{
  public:
    PS2Keyboard()
        : Device(DriverType(0), DeviceType(0))
    {
    }
    PS2Keyboard(PS2Port port)
        : Device({}, {})
    {
        Initialize(port);
    }

    void                     Initialize(PS2Port port);

    virtual std::string_view GetName() const noexcept { return "ps2keyboard"; }

    virtual isize Read(void* dest, off_t offset, usize bytes) { return -1; }
    virtual isize Write(const void* src, off_t offset, usize bytes)
    {
        return -1;
    }

    virtual i32 IoCtl(usize request, uintptr_t argp) { return -1; }

  private:
    CTOS_UNUSED PS2Port m_Port = PS2Port::eNone;

    enum class Command
    {
        eSetLED                         = 0xed,
        eEcho                           = 0xee,
        eIdentify                       = 0xf2,
        eSetTypematicRateAndDelay       = 0xf3,
        eEnableScanning                 = 0xf4,
        eDisableScanning                = 0xf5,
        eSetDefaultParameters           = 0xf6,
        // Only scancode set 3
        eSetAllKeysToAutoRepeat         = 0xf7,
        // --||--
        eSetKeysToMakeRelease           = 0xf8,
        //  --||--
        eSetKeysToMakeOnly              = 0xf9,
        // --||--
        eSetKeysToAutoRepeatMakeRelease = 0xfa,
        // --||--
        eSetKeyToAutoRepeat             = 0xfb,
        // --||--
        eSetKeyToMakeRelease            = 0xfc,
        // --||--
        eSetKeyToMakeOnly               = 0xfd,
        eResendByte                     = 0xfe,
        eResetAndStartSelfTest          = 0xff,
    };

    enum class Response
    {
        eKeyDetectionError = 0x00,
        eSelfTestPassed    = 0xaa,
        eEchoResponse      = 0xee,
        eAcknowledge       = 0xfa,
    };

    static void HandleInterrupt(CPUContext* ctx);
};
