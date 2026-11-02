/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Library/Module.hpp>
#include <Compiler.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/UnixTypes.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

module Cryptix:Impl.Library.Module;

import :Library.Module;

Module* Module::s_ThisModule = nullptr;

namespace System
{
    Ref<Module> FindModule(StringView name);
};

void Module::ParseModuleInfo()
{
    usize       stringSectionIndex = Image->Header().SectionNamesIndex;
    const auto& stringTableSection = Image->SectionHeader(stringSectionIndex);
    const char* sectionNames
        = Image->Raw().Offset<const char*>(stringTableSection.Offset);

    ELF::SectionHeader* modInfoSection   = nullptr;
    ELF::SectionHeader* modParamsSection = nullptr;
    for (usize i = 0; i < Image->SectionHeaderCount(); i++)
    {
        const auto& section     = Image->SectionHeader(i);
        StringView  sectionName = sectionNames + section.Name;

        if (sectionName.StartsWith(".module_init"_sv))
        {
            auto& preludium
                = *Image->Raw().Offset<ModulePreludium*>(section.Offset);
            Preludium = &preludium;
            Name      = Preludium->Name;
        }
        else if (sectionName.StartsWith(".modinfo"_sv))
        {
            modInfoSection = const_cast<ELF::SectionHeader*>(&section);
            break;
        }
        else if (sectionName.StartsWith(".modparams"))
        {
            modParamsSection = const_cast<ELF::SectionHeader*>(&section);
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

            if (key == "name"_sv) Name = value;
            else if (key == "author"_sv)
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
            else if (key == "softdep"_sv)
            {
                StringView depsView = value;
                auto       pre      = "pre: "_sv;
                if (depsView.StartsWith(pre)) depsView.RemovePrefix(pre.Size());
                if (depsView.EndsWith(":")) depsView.RemoveSuffix(1);

                LogDebug("ModInfo[softdep] => {}, {}", value, depsView);
                Dependencies[depsView] = nullptr;
            }

            pos = entryEnd + 1;
        }
    }

    if (!modParamsSection) return;
    auto sectionAddr = modParamsSection->Address;
    auto sectionSize = modParamsSection->Size;

    if (sectionSize % sizeof(ModuleParameter) != 0)
    {
        Failed = true;
        return;
    }

    usize count = sectionSize / sizeof(ModuleParameter);
    Parameters  = Span<ModuleParameter, DynamicExtent>(
        reinterpret_cast<ModuleParameter*>(sectionAddr), count);
}
void Module::Prepare()
{
    for (auto f : InitArray)
        if (f) f();
    State = ModuleState::eReady;
}
ErrorOr<void> Module::Dispatch()
{
    for (auto& [name, module] : Dependencies)
    {
        if (!module) continue;
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
