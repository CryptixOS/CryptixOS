/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ELF.hpp>
#include <Library/Logger.hpp>
#include <Library/Module.hpp>

#include <Memory/VMM.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <magic_enum/magic_enum.hpp>

static std::unordered_map<StringView, Module*> s_Modules;
static Vector<ELF::Image*>                     s_LoadedModules;

std::unordered_map<StringView, Module*>& GetModules() { return s_Modules; }

static void                              LoadModuleFromFile(INode* node)
{
    LogTrace("Module: Loading '{}'...", node->GetPath());
    Vector<u8> elf;
    elf.Resize(node->GetStats().st_size);
    node->Read(elf.Raw(), 0, node->GetStats().st_size);

    ELF::Image* image = new ELF::Image();
    if (!image->LoadFromMemory(elf.Raw(), elf.Size()))
    {
        LogError("Module: Failed to load '{}'", node->GetPath());
        return;
    }
    s_LoadedModules.PushBack(image);
}
bool Module::Load()
{
    INode* moduleDirectory
        = VFS::ResolvePath(VFS::GetRootNode(), "/lib/modules/").Node;
    if (!moduleDirectory) return false;

    for (const auto& [name, child] : moduleDirectory->GetChildren())
        LoadModuleFromFile(child);

    return true;
}

bool AddModule(Module* drv, StringView name)
{
    if (s_Modules.contains(name))
    {
        LogError("Module: '{}' already exists, aborting...", name);
        return false;
    }

    s_Modules[name] = drv;
    drv->Initialize();
    return true;
}
bool LoadModule(Module* drv)
{
    StringView name(drv->Name);

    LogInfo("Module: Successfully loaded '{}'", name);
    return AddModule(drv, name);
}
