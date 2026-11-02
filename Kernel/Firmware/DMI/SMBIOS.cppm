/*
 * Created by v1tr10l7 on 16.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Memory/Pointer.hpp>

export module Cryptix:Firmware.DMI.SMBIOS;

export namespace DMI::SMBIOS
{
    void Initialize(Pointer entry32, Pointer entry64);
};
