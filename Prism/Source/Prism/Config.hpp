/*
 * Created by v1tr10l7 on 21.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

namespace Prism
{
};

#ifdef PRISM_TARGET_CRYPTIX
    #define CryptixNameSpace using namespace Prism;
#else
    #define CryptixNameSpace
#endif

namespace PM = Prism;
