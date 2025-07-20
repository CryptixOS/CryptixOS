/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/PCI/Device.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Prism/Core/Types.hpp>

namespace PCI
{
    struct Driver
    {
        String         Name;
        Span<DeviceID> MatchIDs;

        using ProbeFn
            = ErrorOr<void> (*)(DeviceAddress& address, const DeviceID& id);
        using RemoveFn = void (*)(Device& device);

        ProbeFn  Probe;
        RemoveFn Remove;
    };

    void                  Initialize();
    void                  InitializeIrqRoutes();

    class HostController* GetHostController(u32 domain);
    bool                  RegisterDriver(struct Driver& driver);

    Device*               FindDeviceByID(const DeviceID& id);

    constexpr usize       PCI_CONFIG_ADDRESS = 0x0cf8;
    constexpr usize       PCI_CONFIG_DATA    = 0x0cfc;

    constexpr u16         PCI_INVALID        = 0xffff;
}; // namespace PCI
