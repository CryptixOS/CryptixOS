/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <stdio.h>
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

module Cryptix:Impl.Library.KLibC.stdio;

import :Debug.Assertions;
import :Library.Logger;
import :Library.Stacktrace;

extern "C"
{
    FILE* stdout = (FILE*)&stdout;
    FILE* stderr = (FILE*)stderr;

    i32   fprintf(FILE* stream, const char* format, ...) { return -1; }
}
