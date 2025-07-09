/*
 * Created by v1tr10l7 on 09.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Stacktrace.hpp>

#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/UnorderedMap.hpp>

#include <Prism/Memory/Pointer.hpp>

namespace ELF
{
    class Image;
}
namespace System
{
    ErrorOr<void>                        LoadKernelSymbols();

    const ELF::Image&                    KernelImage();
    const RedBlackTree<StringView, u64>& KernelSymbols();

    Pointer                              LookupKernelSymbol(StringView name);
}; // namespace System
