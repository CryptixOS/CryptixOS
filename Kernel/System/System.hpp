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
#include <Prism/Core/Iterator.hpp>

#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/PathView.hpp>
#include <System/Resource.hpp>

namespace ELF
{
    class Image;
}

class DirectoryEntry;
struct BootModuleInfo;
using ModuleIterator = Delegate<IterationResult(Ref<Module> module)>;

namespace ELF
{
    class Loader;
};
namespace System
{
    ErrorOr<void> LoadKernelSymbols(const BootModuleInfo& kernelExecutable);
    void          PrepareBootModules(Span<BootModuleInfo> bootModules);
    const BootModuleInfo*                FindBootModule(StringView name);
    void                                 InitializeNumaDomains();

    ErrorOr<void>                        LoadBuiltinModules();
    ErrorOr<void>                        LoadExternalModules();

    ErrorOr<void>                        LoadModule(PathView path);
    ErrorOr<void>                        LoadModule(ELF::Loader& loader);
    ErrorOr<void>                        LoadModule(Ref<Module> module);

    ErrorOr<void>                        DispatchModules();

    void                                 ForEachModule(ModuleIterator iterator);
    Ref<Module>                          FindModule(StringView name);

    PathView                             KernelExecutablePath();
    const ELF::Image&                    KernelImage();
    const RedBlackTree<StringView, u64>& KernelSymbols();

    u64                                  LookupKernelSymbol(StringView name);

#if CTOS_TARGET_X86_64
    ErrorOr<IoPortResource*> AllocateIoPortAt(Module* owner, u16 base,
                                              u16 length);
#endif
}; // namespace System
