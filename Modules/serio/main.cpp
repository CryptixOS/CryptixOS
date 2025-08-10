/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/CharacterDevice.hpp>
#include <Drivers/Core/GenericDriver.hpp>
#include <Library/Module.hpp>

// class SerialController : public CharacterDevice
// {
//   public:
//     static ErrorOr<void> Probe() { return Error(ENOSYS); }
//     static void          Remove() {}
// };
//
// static GenericDriver s_Driver = {
//     .Name   = "serio"_s,
//     .Probe  = SerialController::Probe,
//     .Remove = SerialController::Remove,
// };

extern "C" CTOS_EXPORT bool ModuleInit()
{
    LogTrace("serio: module init");
    return true;
    // return RegisterGenericDriver(s_Driver);
}
MODULE_INIT(serio, ModuleInit);
