/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Logger.hpp>
#include <Library/Module.hpp>

#include <magic_enum/magic_enum.hpp>
#include <unordered_map>

static std::unordered_map<std::string_view, Module*> list;

bool AddModule(Module* drv, std::string_view name)
{
    if (list.contains(name))
    {
        LogError("Module: '{}' already exists, aborting...", name);
        return false;
    }

    list[name] = drv;
    drv->Initialize();
    return true;
}
bool LoadModule(Module* drv)
{
    std::string_view name(drv->Name);

    LogInfo("Module: Successfully loaded '{}'", name);
    return AddModule(drv, name);
}
