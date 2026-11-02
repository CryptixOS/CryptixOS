/*
 * Created by v1tr10l7 on 26.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/String/StringView.hpp>

export module Cryptix:Debug.KernelPanic;

export enum class IrqResult;

export class Device;
export [[noreturn]]
IrqResult HaltAndCatchFire(class Device*, struct ExecutionContext* context);

export [[noreturn]]
void earlyPanic(const char* format, ...);
