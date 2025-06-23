/*
 * Created by v1tr10l7 on 26.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/String/StringView.hpp>

[[noreturn]]
void HaltAndCatchFire(struct CPUContext* context);

[[noreturn]]
void panic(StringView message);
[[noreturn]]
void earlyPanic(const char* format, ...);
