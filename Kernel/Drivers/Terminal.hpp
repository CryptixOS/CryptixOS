/*
 * Created by v1tr10l7 on 23.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Boot/BootInfo.hpp>
#include <Prism/Core/Types.hpp>

#include <Prism/Spinlock.hpp>

#include <flanterm.h>
#include <string_view>
#include <vector>

namespace AnsiColor
{
    constexpr const char* FOREGROUND_BLACK   = "\u001b[30m";
    constexpr const char* FOREGROUND_RED     = "\u001b[31m";
    constexpr const char* FOREGROUND_GREEN   = "\u001b[32m";
    constexpr const char* FOREGROUND_YELLOW  = "\u001b[33m";
    constexpr const char* FOREGROUND_BLUE    = "\u001b[34m";
    constexpr const char* FOREGROUND_MAGENTA = "\u001b[35m";
    constexpr const char* FOREGROUND_CYAN    = "\u001b[36m";
    constexpr const char* FOREGROUND_WHITE   = "\u001b[37m";

    constexpr const char* BACKGROUND_BLACK   = "\u001b[40m";
    constexpr const char* BACKGROUND_RED     = "\u001b[41m";
    constexpr const char* BACKGROUND_GREEN   = "\u001b[42m";
    constexpr const char* BACKGROUND_YELLOW  = "\u001b[43m";
    constexpr const char* BACKGROUND_BLUE    = "\u001b[44m";
    constexpr const char* BACKGROUND_MAGENTA = "\u001b[45m";
    constexpr const char* BACKGROUND_CYAN    = "\u001b[46m";
    constexpr const char* BACKGROUND_WHITE   = "\u001b[47m";
}; // namespace AnsiColor

class Terminal final
{
  public:
    Terminal() = default;
    Terminal(Framebuffer& m_Framebuffer, usize id)
        : m_ID(id)
    {
        Initialize(m_Framebuffer);
    }

    bool                      Initialize(Framebuffer& m_Framebuffer);

    inline const Framebuffer& GetFramebuffer() const { return m_Framebuffer; }
    inline const flanterm_context* GetContext() const { return m_Context; }

    void                           Clear(u32 color = 0xffffff);
    void                           PutChar(u64 c);

    void                           PrintString(std::string_view str);

    static Terminal*               GetPrimary();
    static std::vector<Terminal*>& EnumerateTerminals();

  private:
    usize                         m_ID          = 0;
    bool                          m_Initialized = false;
    Framebuffer                   m_Framebuffer = {};
    flanterm_context*             m_Context     = nullptr;
    Spinlock                      m_Lock;

    static std::vector<Terminal*> s_Terminals;
};
