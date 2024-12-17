
/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Serial.hpp>
#include <Drivers/Terminal.hpp>

#include <Memory/PMM.hpp>
#include <Utility/BootInfo.hpp>

#include <backends/fb.h>

#include <cstdlib>

std::vector<Terminal*> Terminal::s_Terminals = {};
Terminal*              s_ActiveTerminal      = nullptr;

bool                   Terminal::Initialize(Framebuffer& framebuffer)
{
    m_Framebuffer = framebuffer;
    if (!framebuffer.address) return false;
    if (m_Initialized) return true;

    auto _malloc = PMM::IsInitialized() ? malloc : nullptr;
    auto _free   = PMM::IsInitialized() ? [](void* addr, usize) { free(addr); }
                                        : nullptr;

    // TODO(v1tr10l7): install callback to flanterm context
    m_Context    = flanterm_fb_init(
        _malloc, _free, reinterpret_cast<u32*>(framebuffer.address),
        framebuffer.width, framebuffer.height, framebuffer.pitch,
        framebuffer.red_mask_size, framebuffer.red_mask_shift,
        framebuffer.green_mask_size, framebuffer.green_mask_shift,
        framebuffer.blue_mask_size, framebuffer.blue_mask_shift, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, 1,
        0, 0, 0);

    if (!s_ActiveTerminal) s_ActiveTerminal = this;
    return (m_Initialized = true);
}

void Terminal::Clear(u32 color)
{
    for (u32 ypos = 0; ypos < m_Framebuffer.height; ypos++)
    {
        for (u32 xpos = 0; xpos < m_Framebuffer.width; xpos++)
        {
            uintptr_t framebufferBase
                = reinterpret_cast<uintptr_t>(m_Framebuffer.address);
            u64  pitch = m_Framebuffer.pitch;
            u8   bpp   = m_Framebuffer.bpp;
            u32* pixel = reinterpret_cast<u32*>(framebufferBase + ypos * pitch
                                                + (xpos * bpp / 8));

            *pixel     = color;
        }
    }
}
void Terminal::PutChar(u64 c)
{
    if (!m_Initialized) return;
    flanterm_write(m_Context, reinterpret_cast<char*>(&c), 1);
}
void Terminal::PrintString(std::string_view str)
{
    std::unique_lock guard(m_Lock);
    for (auto c : str) PutChar(c);
}

std::vector<Terminal*>& Terminal::EnumerateTerminals()
{
    static bool initialized = false;
    if (initialized) return s_Terminals;

    usize         framebufferCount = 0;
    Framebuffer** framebuffers = BootInfo::GetFramebuffers(framebufferCount);

    for (usize i = 0; i < framebufferCount; i++)
    {
        if (s_ActiveTerminal && s_ActiveTerminal->m_ID == i)
        {
            s_Terminals.push_back(s_ActiveTerminal);
            continue;
        }

        s_Terminals.push_back(new Terminal(*framebuffers[i], i));
    }

    LogInfo("Terminal: Initialized {} terminals", s_Terminals.size());
    initialized = true;
    return s_Terminals;
}
