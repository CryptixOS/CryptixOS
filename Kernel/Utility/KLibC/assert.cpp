/*
 * Created by v1tr10l7 on 22.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "assert.h"

#include "Common.hpp"

extern "C"
{
    __attribute__((noreturn)) extern void __assert_fail(const char*  expr,
                                                        const char*  file,
                                                        unsigned int line,
                                                        const char*  function)
    {
        Panic("Assertion Failed({}::{}:{}): {}", file, function, line, expr);
    }

    __attribute__((noreturn)) void __assert(const char* expr,
                                            const char* filename, int line)
    {
        __assert_fail(expr, filename, line, "");
    }
}
