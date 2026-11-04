/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>

export module Cryptix:Arch.x86_64.Drivers.PCSpeaker;

export namespace PCSpeaker
{
    void ToneOn(u64 frequency);
    void ToneOff();
}; // namespace PCSpeaker
