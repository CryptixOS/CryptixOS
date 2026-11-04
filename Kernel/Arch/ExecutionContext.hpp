/*
 * Created by v1tr10l7 on 15.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#ifdef CTOS_TARGET_X86_64
#elifdef CTOS_TARGET_AARCH64
#else
    #error "Unsupported cpu architecture!"
#endif
