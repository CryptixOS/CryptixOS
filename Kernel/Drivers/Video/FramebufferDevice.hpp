/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/linux/fb.h>

#include <Drivers/Core/CharacterDevice.hpp>
#include <Drivers/Video/Framebuffer.hpp>

class FramebufferDevice : public CharacterDevice
{
  public:
    FramebufferDevice(StringView name, Framebuffer& framebuffer);

    static bool            Initialize();
    virtual StringView     Name() const noexcept override;

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override;

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override;
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override;

    virtual i32            IoCtl(usize request, uintptr_t argp) override;

  private:
    Framebuffer       m_Framebuffer;
;
    fb_var_screeninfo m_VariableScreenInfo;
    fb_fix_screeninfo m_FixedScreenInfo;
};
