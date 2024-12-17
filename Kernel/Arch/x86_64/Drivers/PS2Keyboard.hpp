/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/x86_64/Drivers/I8042Controller.hpp>

#include <Drivers/Device.hpp>

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

    static void         HandleInterrupt(CPUContext* ctx);
};
