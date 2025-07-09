/*
 * Created by v1tr10l7 on 09.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootInfo.hpp>

#include <Library/ELF.hpp>
#include <System/System.hpp>

namespace System
{
    namespace
    {
        ELF::Image                    s_KernelImage;
        RedBlackTree<StringView, u64> s_KernelSymbols;
    }; // namespace

    ErrorOr<void> LoadKernelSymbols()
    {
        LogTrace("System: Loading kernel symbols...");

        auto kernelExecutable = BootInfo::ExecutableFile();
        if (!kernelExecutable || !kernelExecutable->address
            || !kernelExecutable->size)
        {
            LogTrace("System: Failed to locate the kernel executable");
            return Error(ENOENT);
        }

        Pointer address = kernelExecutable->address;
        usize   size    = kernelExecutable->size;
        LogTrace(
            "System: Loaded the kernel executable with size `{}`KiB at `{:#x}`",
            size / 1024, address.Raw());

        s_KernelImage.LoadFromMemory(address, size);
        ELF::Image::SymbolEnumerator it;
        it.BindLambda(
            [&](ELF::Sym& symbol, StringView name) -> bool
            {
                if (name.Empty()) return true;

                s_KernelSymbols[name] = symbol.Value;
                return true;
            });

        s_KernelImage.ForEachSymbol(it);
        LogTrace("System: Successfully loaded the kernel symbols");

        return {};
    }

    const ELF::Image&                    KernelImage() { return s_KernelImage; }
    const RedBlackTree<StringView, u64>& KernelSymbols()
    {
        return s_KernelSymbols;
    }

    Pointer LookupKernelSymbol(StringView name)
    {
        auto it = s_KernelSymbols.Find(name);
        if (it == s_KernelSymbols.end()) return nullptr;

        return it->Value;
    }
}; // namespace System
