/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/PCI/PCI.hpp>
#include <Drivers/PCI/HostController.hpp>
#include <Drivers/PCI/Device.hpp>
#include <Drivers/USB/USB.hpp>
#include <Drivers/USB/UHCI/UHCiController.hpp>
#include <Library/Logger.hpp>

namespace USB
{
    static HostController::List s_HostControllers{};

    ErrorOr<void>               Initialize()
    {
        LogTrace("USB: Initializing...");
        auto            pciController = PCI::GetHostController(0);

        PCI::Enumerator enumerator;
        enumerator.BindLambda(
            [pciController](const PCI::DeviceAddress& addr) -> bool
            {
                u8 classID = pciController->Read<u8>(
                    addr, ToUnderlying(PCI::RegisterOffset::eClassID));
                u8 subClassID = pciController->Read<u8>(
                    addr, ToUnderlying(PCI::RegisterOffset::eSubClassID));
                u8 progIf = pciController->Read<u8>(
                    addr, ToUnderlying(PCI::RegisterOffset::eProgIf));

                if (classID == 0x0c && subClassID == 0x03 && progIf == 0x00) 
                {
                    LogInfo("USB: Discovered UHCI controller on the PCI bus => {:#04x}:{:#04x}:{:#04x}:{:#04x}", addr.Domain, addr.Bus, addr.Slot, addr.Function);
                    if (!USB::RegisterController(new USB::UHCI::Controller(addr)))
                        LogError("USB: Failed to register USB UHCI controller");
                    else LogInfo("USB: Successfully registered new USB UHCI controller");

                    return true;
                }
                
                return false;
            });

        if (!pciController->EnumerateDevices(enumerator))
        {
            LogWarn("USB: Failed to detect any host controllers");
            return Error(ENODEV);
        }
        return {};
    }
    ErrorOr<void> RegisterController(HostController* controller)
    {
        LogTrace("USB: Registering the host controller...");
        auto status = controller->Initialize();

        if (!status)
        {
            LogError(
                "USB: Failed to initialize the host controller, aborting...");
            return Error(status.error());
        }

        s_HostControllers.PushBack(controller);
        LogInfo("USB: Initialized controller count => {}",
                s_HostControllers.Size());

        return {};
    }
}; // namespace USB
