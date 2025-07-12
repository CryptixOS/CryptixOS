/*
 * Created by v1tr10l7 on 09.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Module.hpp>
#include <Library/Stacktrace.hpp>

#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/UnorderedMap.hpp>

#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/PathView.hpp>

namespace ELF
{
    class Image;
}

class DirectoryEntry;
struct BootModuleInfo;
namespace System
{
    ErrorOr<void> LoadKernelSymbols(const BootModuleInfo& kernelExecutable);
    void          PrepareBootModules(Span<BootModuleInfo> bootModules);
    const BootModuleInfo*                FindBootModule(StringView name);

    ErrorOr<void>                        LoadModules();

    ErrorOr<void>                        LoadModule(PathView path);
    ErrorOr<void>                        LoadModule(Ref<DirectoryEntry> entry);
    ErrorOr<void>                        LoadModule(Ref<Module> module);

    Module::List&                        Modules();
    Ref<Module>                          FindModule(StringView name);

    PathView                             KernelExecutablePath();
    const ELF::Image&                    KernelImage();
    const RedBlackTree<StringView, u64>& KernelSymbols();

    u64                                  LookupKernelSymbol(StringView name);
}; // namespace System
