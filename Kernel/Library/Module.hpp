/*
 * Created by v1tr10l7 on 04.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#pragma once

#include <Compiler.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#define MODULE_INIT(name_, init)                                               \
    CTOS_MODULE_INFO_STRING(name, #name_);                                     \
    extern "C" MODULE_SECTION const ModulePreludium CtConcatenateName(         \
        kernel_module, CtUniqueName(name_))                                    \
        = {.Name = #name_, .Initialize = init, .Terminate = nullptr}
#define MODULE_EXIT(name, exit)                                                \
    extern "C" CTOS_SECTION(MODULE_SECTION_NAME "." #name,                     \
                            CTOS_FORCE_EMIT) void (*Terminate)()               \
        = exit;
#define CTOS_MODULE_INFO_STRING(name, value)                                   \
    CTOS_SECTION_FORCE_EMIT(".modinfo")                                        \
    alignas(0x01) static const char CtUniqueName(modinfo)[]                    \
        = CtStringify(name) "=" value;
#define CTOS_MODULE_AUTHOR(author_) CTOS_MODULE_INFO_STRING(author, author_)
#define CTOS_MODULE_DESCRIPTION(description_)                                  \
    CTOS_MODULE_INFO_STRING(description, description_)
#define CTOS_MODULE_LICENSE(license_) CTOS_MODULE_INFO_STRING(license, license_)
#define CTOS_MODULE_VERSION(version_) CTOS_MODULE_INFO_STRING(version, version_)
#define CTOS_MODULE_SOFTDEP(deps)     CTOS_MODULE_INFO_STRING(softdep, deps)
#define CTOS_MODULE_PARAM(name, type, default_val, desc)                       \
    static type __param_##name = default_val;                                  \
    CTOS_SECTION_FORCE_EMIT(".modparams")                                      \
    static const ModuleParameter __pdesc_##name                                \
        = {.Name        = #name, /* Assumes type matches enum */               \
           .Type        = ModuleParamType::e##type,                            \
           .Address     = &__param_##name,                                     \
           .Description = desc};
#define CTOS_THIS_MODULE() Module::ThisModule()
