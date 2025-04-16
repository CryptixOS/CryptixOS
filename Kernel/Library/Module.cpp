/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Logger.hpp>
#include <Library/Module.hpp>

#include <Memory/VMM.hpp>

#include <magic_enum/magic_enum.hpp>

static std::unordered_map<StringView, Module*> s_Modules;

std::unordered_map<StringView, Module*>& GetModules() { return s_Modules; }

bool                                     AddModule(Module* drv, StringView name)
{
    if (s_Modules.contains(name))
    {
        LogError("Module: '{}' already exists, aborting...", name);
        return false;
    }

    s_Modules[name] = drv;
    /*
    VMM::GetKernelPageMap()->MapRange(Pointer(drv->Initialize),
                                      Pointer(drv->Initialize).FromHigherHalf(),
                                      0x1000 * 10, PageAttributes::eRWX);
    */
    drv->Initialize();
    return true;
}
bool LoadModule(Module* drv)
{
    StringView name(drv->Name);

    LogInfo("Module: Successfully loaded '{}'", name);
    return AddModule(drv, name);
}
