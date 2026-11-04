/*
 * Created by v1tr10l7 on 09.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#pragma once

#include <Compiler.hpp>
#include <Prism/Core/Types.hpp>
#define Asm(...) __asm__ volatile(__VA_ARGS__)
#define ReadSystemRegister(register)                                           \
    ({                                                                         \
        u64 value;                                                             \
        Asm("mrs %0, " CtStringify(register) : "=r"(value));                   \
        value;                                                                 \
    })
#define WriteSystemRegister(newValue, register)                                \
    do {                                                                       \
        u64 value = static_cast<u64>(newValue);                                \
        Asm("msr " CtStringify(register) ", %x0" : : "rZ"(value));             \
    } while (0)
