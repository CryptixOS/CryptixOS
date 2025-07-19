/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Compiler.hpp>

#include <Library/ELF.hpp>

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Containers/UnorderedMap.hpp>

#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>

#include <uacpi/resources.h>
#include <uacpi/utilities.h>

namespace ELF
{
    class Image;
}

using ModuleInitProc      = bool (*)();
using ModuleTerminateProc = void (*)();
struct CTOS_PACKED_ALIGNED(8) ModuleHeader
{
    const char*         Name;

    ModuleInitProc      Initialize;
    ModuleTerminateProc Terminate;
};
struct Module : public RefCounted
{
    StringView Name;
    bool       Initialized;
    bool       Failed;

    friend class IntrusiveRefList<Module>;
    friend struct IntrusiveRefListHook<Module>;

    using List = IntrusiveRefList<Module>;
    IntrusiveRefListHook<Module> Hook;

    ::Ref<ELF::Image>            Image = nullptr;

    ModuleInitProc               Initialize;
    ModuleTerminateProc          Terminate;

    static bool                  Load();
};

#define MODULE_INIT(name, init)                                                \
    extern "C" MODULE_SECTION const ModuleHeader CtConcatenateName(            \
        kernel_module, CtUniqueName(name))                                     \
        = {.Name = #name, .Initialize = init, .Terminate = nullptr}

#define MODULE_EXIT(name, exit)                                                \
    extern "C" CTOS_SECTION(MODULE_SECTION_NAME "." #name,                     \
                            CTOS_FORCE_EMIT) void (*Terminate)()               \
        = exit;
