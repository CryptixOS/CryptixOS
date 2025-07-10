/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/ELF.hpp>

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Containers/UnorderedMap.hpp>

#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>

#include <uacpi/resources.h>
#include <uacpi/utilities.h>

#define CONCAT(a, b)       CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a##b
#define UNIQUE_NAME(name)  CONCAT(_##name##__, CONCAT(__COUNTER__, __LINE__))

namespace ELF
{
    class Image;
}

using ModuleInitProc      = bool (*)();
using ModuleTerminateProc = void (*)();
struct [[gnu::packed, gnu::aligned(8)]] ModuleHeader
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

#define MODULE_SECTION      ".module_init"
#define MODULE_DATA_SECTION ".module_init.data"

#define MODULE_INIT(name, init)                                                \
    extern "C" [[gnu::section(MODULE_SECTION), gnu::used]] const ModuleHeader  \
    CONCAT(kernel_module, UNIQUE_NAME(name))                                   \
        = {.Name = #name, .Initialize = init, .Terminate = nullptr}

#define MODULE_EXIT(name, exit)                                                \
    extern "C" [[gnu::section(MODULE_SECTION "." #name)], gnu::used]] \
    void (*Terminate)() = exit;
