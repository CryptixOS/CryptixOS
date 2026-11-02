/*
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
// Prism, the demangler and the C++ runtime are not part of the `Cryptix` module.  They declare
// the functions below in the global module and call them by those names, so the definitions
// have to be global-module entities as well.  Everything declared by a unit of `Cryptix` is
// attached to the module and mangled differently, which is why these are thin forwarders that
// call into the kernel through extern "C" entry points.
//
// This translation unit deliberately does not import `Cryptix`.
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>

extern "C"
{
    [[noreturn]] void CryptixKernelPanic(const char* message, usize size);
    isize              CryptixLoggerPrint(const char* message, usize size);
}

// Declared by <Prism/Debug/Assertions.hpp>, behind PrismPanic().
[[noreturn]] void panic(StringView message)
{
    CryptixKernelPanic(message.Raw(), message.Size());
}

// Called by Prism::Log::Print.
namespace Logger
{
    isize Print(StringView string)
    {
        return CryptixLoggerPrint(string.Raw(), string.Size());
    }
} // namespace Logger

namespace std
{
    [[gnu::noreturn]] void terminate() noexcept
    {
        CryptixKernelPanic("std::terminate()", 16);
    }
} // namespace std
