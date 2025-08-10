/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Module.hpp>

void Module::ParseModuleInfo()
{
    usize       stringSectionIndex = Image->Header().SectionNamesIndex;
    auto        stringTableSection = Image->SectionHeader(stringSectionIndex);
    const char* sectionNames
        = Image->Raw().Offset<const char*>(stringTableSection->Offset);

    ELF::SectionHeader* modInfoSection = nullptr;
    for (usize i = 0; i < Image->SectionHeaderCount(); i++)
    {
        auto&      section     = *Image->SectionHeader(i);
        StringView sectionName = sectionNames + section.Name;

        if (sectionName.StartsWith(".modinfo"_sv))
        {
            modInfoSection = &section;
            break;
        }
    }

    if (!modInfoSection) return;
    StringView modInfo(Image->Raw().Offset<const char*>(modInfoSection->Offset),
                       modInfoSection->Size);
    usize      pos = 0;
    while (pos < modInfoSection->Size)
    {
        usize entryEnd = modInfo.Find('\0', pos);
        if (entryEnd == StringView::NPos) break;

        StringView entry = modInfo.Substr(pos, entryEnd - pos);
        if (entry.Empty()) break;

        usize equalPos = entry.Find('=');
        if (equalPos != StringView::NPos)
        {
            StringView key   = entry.Substr(0, equalPos);
            StringView value = entry.Substr(equalPos + 1);

            if (key == "author"_sv)
            {
                Info.Author = value;
                LogDebug("ModInfo[author] => {}", value);
            }
            else if (key == "description"_sv)
            {
                Info.Description = value;
                LogDebug("ModInfo[description] => {}", value);
            }
            else if (key == "license"_sv)
            {
                Info.License = value;
                LogDebug("ModInfo[license] => {}", value);
            }
            else if (key == "version"_sv)
            {
                Info.Version = value;
                LogDebug("ModInfo[version] => {}", value);
            }

            pos = entryEnd + 1;
        }
    }
}

void Module::Prepare()
{
    for (auto f : InitArray)
        if (f) f();
    State = ModuleState::eReady;
}
ErrorOr<void> Module::Dispatch()
{
    for (auto module : Dependencies)
    {
        switch (module->State)
        {
            case ModuleState::eLoaded: module->Prepare(); CTOS_FALLTHROUGH;
            case ModuleState::eReady: module->Dispatch(); break;

            case ModuleState::eEmpty: return Error(ENOEXEC);
            default: break;
        }

        if (module->State != ModuleState::eRunning) return Error(ENOEXEC);
    }

    if (Initialize())
    {
        State = ModuleState::eRunning;
        return {};
    }

    return Error(ENOEXEC);
}
void Module::Unload()
{
    if (State == ModuleState::eRunning)
    {
        Terminate();
        for (auto f : FiniArray)
            if (f) f();
        State = ModuleState::eUnloaded;
    }
}
