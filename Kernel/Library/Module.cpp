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
#include <VFS/DirectoryEntry.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <magic_enum/magic_enum.hpp>

static std::unordered_map<StringView, Module*> s_Modules;
static Vector<ELF::Image*>                     s_LoadedModules;

std::unordered_map<StringView, Module*>& GetModules() { return s_Modules; }

static void LoadModuleFromFile(DirectoryEntry* entry)
{
    Assert(entry);
    auto inode = entry->INode();
    Assert(inode);

    LogTrace("Module: Loading '{}'...", entry->Path());
    Vector<u8> elf;
    elf.Resize(inode->Stats().st_size);
    inode->Read(elf.Raw(), 0, inode->Stats().st_size);

    ELF::Image* image = new ELF::Image();
    if (!image->LoadFromMemory(elf.Raw(), elf.Size()))
    {
        LogError("Module: Failed to load '{}'", entry->Path());
        return;
    }
    s_LoadedModules.PushBack(image);
}
bool Module::Load()
{
    auto moduleDirectory
        = VFS::ResolvePath(VFS::GetRootDirectoryEntry(), "/lib/modules/").Entry;
    if (!moduleDirectory) return false;

    for (const auto& [name, child] : moduleDirectory->Children())
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
