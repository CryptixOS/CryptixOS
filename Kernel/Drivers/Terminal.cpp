
/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Terminal.hpp"
#include "Utility/BootInfo.hpp"

#include <backends/fb.h>

#include <cstring>

bool Terminal::Initialize(Framebuffer& framebuffer)
{
    this->framebuffer = framebuffer;
    if (!framebuffer.address) return false;

    context = flanterm_fb_init(
        nullptr, nullptr, reinterpret_cast<u32*>(framebuffer.address),
        framebuffer.width, framebuffer.height, framebuffer.pitch,
        framebuffer.red_mask_size, framebuffer.red_mask_shift,
        framebuffer.green_mask_size, framebuffer.green_mask_shift,
        framebuffer.blue_mask_size, framebuffer.blue_mask_shift, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, 1,
        0, 0, 0);

    return (initialized = true);
}

void Terminal::Clear(u32 color)
{
    for (u32 ypos = 0; ypos < framebuffer.height; ypos++)
    {
        for (u32 xpos = 0; xpos < framebuffer.width; xpos++)
        {
            uintptr_t framebufferBase
                = reinterpret_cast<uintptr_t>(framebuffer.address);
            u64  pitch = framebuffer.pitch;
            u8   bpp   = framebuffer.bpp;
            u32* pixel = reinterpret_cast<u32*>(framebufferBase + ypos * pitch
                                                + (xpos * bpp / 8));

            *pixel     = color;
        }
    }
}
void Terminal::PutChar(u64 c)
{
    if (!initialized) return;
    flanterm_write(context, reinterpret_cast<char*>(&c), 1);
}
void Terminal::PrintString(const char* string, usize length)
{
    while (length > 0)
    {
        PutChar(*string++);
        --length;
    }
}
void Terminal::PrintString(const char* string)
{
    while (*string != '\0') { PutChar(*string++); }
}
