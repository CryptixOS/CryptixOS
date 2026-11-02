/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

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

export module Cryptix:Arch.x86_64.IDT;

export import :Debug.Assertions;
export import :Library.Logger;
export import :Library.Stacktrace;

export constexpr usize DPL_RING0 = 0x00;
export constexpr usize DPL_RING3 = 0x03;

export namespace IDT
{
    void Initialize();
    void Load();

    u32  GetIST(u8 vector);
    void SetIST(u8 vector, u32 value);
    void SetDPL(u8 vector, u8 dpl = 3);
}; // namespace IDT
