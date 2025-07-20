/*
 * Created by v1tr10l7 on 16.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Memory/Pointer.hpp>

namespace DMI::SMBIOS
{
    void Initialize(Pointer entry32, Pointer entry64);
};
