/*
 * Created by v1tr10l7 on 08.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/Device.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Delegate.hpp>

class BlockDevice : public Device
{
  public:
    BlockDevice(StringView name, DeviceID id)
        : Device(name, GetDeviceMajor(id), GetDeviceMinor(id))
    {
    }
    BlockDevice(DeviceID id)
        : Device(GetDeviceMajor(id), GetDeviceMinor(id))
    {
    }

    inline constexpr dev_t ID() const { return m_ID; }

    virtual bool           IsBlockDevice() const { return true; }
};
