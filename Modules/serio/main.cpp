/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <SerioController.hpp>

#include <Drivers/Core/GenericDriver.hpp>
#include <Library/Module.hpp>

CTOS_MODULE_AUTHOR("v1tr10l7");
CTOS_MODULE_DESCRIPTION("serio controller driver interface");
CTOS_MODULE_LICENSE("GPL-3");
CTOS_MODULE_VERSION("0.2");

static Ref<SerioController> s_Serio = nullptr;

CTOS_EXPORT CTOS_FORCE_EMIT ErrorOr<void>
SerioController::Register(::Ref<SerioController> ctrl)
{
    if (s_Serio) return Error(EEXIST);
    s_Serio = ctrl;

    return {};
}
CTOS_EXPORT CTOS_FORCE_EMIT ::Ref<SerioController> SerioController::Instance()
{
    return s_Serio;
}

extern "C" CTOS_EXPORT bool ModuleInit()
{
    LogTrace("serio: module successfully initialized");
    return true;
}
MODULE_INIT(serio, ModuleInit);
