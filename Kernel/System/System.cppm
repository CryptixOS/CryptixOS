/*
 * Created by v1tr10l7 on 09.07.2025.
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
#include <Prism/Core/Iterator.hpp>

export module Cryptix:System.System;

export import :Library.Module;
export import :Library.Stacktrace;
export import :System.Resource;
import :Boot.BootModuleInfo;

export namespace ELF
{
    class Image;
}

export class DirectoryEntry;
export struct BootModuleInfo;
export using ModuleIterator = Delegate<IterationResult(Ref<Module> module)>;

export namespace ELF
{
    class Loader;
};
export namespace System
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
