/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/linux/fb.h>

#include <Drivers/Device.hpp>
#include <Library/Color.hpp>

class FramebufferDevice : public Device
{
  public:
    FramebufferDevice(limine_framebuffer* framebuffer);

    static bool        Initialize();

    virtual StringView GetName() const noexcept override
    {
        auto name = fmt::format("fbdev{}", m_ID);
        return StringView(name.data(), name.size());
    }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override;

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override;
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override;

    virtual i32            IoCtl(usize request, uintptr_t argp) override;

  private:
    struct Framebuffer
    {
        Pointer     Address;
        u64         Width;
        u64         Height;
        u64         Pitch;
        u16         BitsPerPixel;
        u8          RedMaskSize;
        u8          RedMaskShift;
        u8          GreenMaskSize;
        u8          GreenMaskShift;
        u8          BlueMaskSize;
        u8          BlueMaskShift;

        inline void PutPixel(usize x, usize y, Color color) const
        {
            u32   red    = color.Red() << RedMaskShift;
            u32   green  = color.Green() << GreenMaskShift;
            u32   blue   = color.Blue() << BlueMaskShift;

            u32   rgb    = red | green | blue;
            usize offset = y * (Pitch / sizeof(u32)) + x;
            Address.As<volatile u32>()[offset] = rgb;
        }

    } m_Framebuffer = {};

    fb_var_screeninfo m_VariableScreenInfo;
    fb_fix_screeninfo m_FixedScreenInfo;
};
