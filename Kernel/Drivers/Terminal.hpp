/*
 * Created by v1tr10l7 on 23.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include <flanterm.h>
#include <string_view>

constexpr const u32 TERMINAL_COLOR_BLACK = 0x000000;

class Terminal      final
{
  public:
    Terminal() = default;
    CTOS_NO_KASAN bool Initialize(Framebuffer& framebuffer);

    void               Clear(u32 color = TERMINAL_COLOR_BLACK);
    void               PutChar(u64 c);

    inline void        PrintString(std::string_view str)
    {
        PrintString(str.data(), str.size());
    }
    void PrintString(const char* string, usize length);
    void PrintString(const char* string);

  private:
    bool              initialized = false;
    Framebuffer       framebuffer = {};
    flanterm_context* context     = nullptr;
};
