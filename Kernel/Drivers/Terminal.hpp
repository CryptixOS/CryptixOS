/*
 * Created by v1tr10l7 on 23.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/BootInfo.hpp"
#include "Utility/Types.hpp"

#include <flanterm.h>
#include <mutex>
#include <string_view>
#include <vector>

constexpr const u32 TERMINAL_COLOR_BLACK = 0x000000;

class Terminal      final
{
  public:
    Terminal() = default;
    Terminal(Framebuffer& framebuffer, usize id)
        : id(id)
    {
        Initialize(framebuffer);
    }

    bool                           Initialize(Framebuffer& framebuffer);

    void                           Clear(u32 color = TERMINAL_COLOR_BLACK);
    void                           PutChar(u64 c);

    void                           PrintString(std::string_view str);

    static std::vector<Terminal*>& EnumerateTerminals();

  private:
    usize                         id          = 0;
    bool                          initialized = false;
    Framebuffer                   framebuffer = {};
    flanterm_context*             context     = nullptr;
    std::mutex                    lock;

    static std::vector<Terminal*> s_Terminals;
};
