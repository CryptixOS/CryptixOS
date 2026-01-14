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
#include <Library/Locking/Mutex.hpp>

class FramebufferDevice : public CharacterDevice
{
  public:
    FramebufferDevice(StringView name, Framebuffer& framebuffer);

    static bool            Initialize();
    static ErrorOr<void>   Register(FramebufferDevice& framebuffer);

    virtual StringView     Name() const noexcept override;

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override;

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override;
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override;

    virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override;

  protected:
    virtual ErrorOr<void> CheckVariableScreenInfo(fb_var_screeninfo& var)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> CheckCapabilities(fb_var_screeninfo& var,
                                            bool               activate)
    {
        return Error(ENOSYS);
    }

  private:
    static Vector<FramebufferDevice*> s_Framebuffers;
    Framebuffer                       m_Framebuffer;

    Atomic<usize>                     m_Count;
    isize                             m_Flags;
    Mutex                             m_Lock;
    Mutex                             m_MemoryLock;
    fb_var_screeninfo                 m_VariableScreenInfo;
    fb_fix_screeninfo                 m_FixedScreenInfo;
    fb_pixmap                         m_Pixmap;
    Vector<fb_videomode>              m_VideoModes;

    char*                             m_ScreenBase;
    usize                             m_ScreenSize;

    ErrorOr<void>                     RemoveVideoMode(const fb_videomode& mode);
    ErrorOr<void>                     SetVar(fb_var_screeninfo& var);
    ErrorOr<void>                     SetUserColorMap(fb_cmap& cmap);

    u32                               GetColorDepth();
    ErrorOr<void>                     CheckForeignness();
};
