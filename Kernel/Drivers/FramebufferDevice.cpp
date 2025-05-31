/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/FramebufferDevice.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

static FramebufferDevice* s_PrimaryFramebuffer = nullptr;

FramebufferDevice::FramebufferDevice(limine_framebuffer* framebuffer)
    : Device(0, 0)
{
    Assert(framebuffer && framebuffer->address);
    m_Framebuffer              = {.Address        = framebuffer->address,
                                  .Width          = framebuffer->width,
                                  .Height         = framebuffer->height,
                                  .Pitch          = framebuffer->pitch,
                                  .BitsPerPixel   = framebuffer->bpp,
                                  .RedMaskSize    = framebuffer->red_mask_size,
                                  .RedMaskShift   = framebuffer->red_mask_shift,
                                  .GreenMaskSize  = framebuffer->green_mask_size,
                                  .GreenMaskShift = framebuffer->green_mask_shift,
                                  .BlueMaskSize   = framebuffer->blue_mask_size,
                                  .BlueMaskShift  = framebuffer->blue_mask_shift};

    m_Stats.st_size            = 0;
    m_Stats.st_blocks          = 0;
    m_Stats.st_blksize         = 4096;
    m_Stats.st_rdev            = 0;
    m_Stats.st_mode            = 0666 | S_IFCHR;

    m_FixedScreenInfo.smem_len = m_FixedScreenInfo.mmio_len
        = m_Framebuffer.Pitch * m_Framebuffer.Height;
    m_FixedScreenInfo.line_length       = m_Framebuffer.Pitch;
    m_FixedScreenInfo.type              = FB_TYPE_PACKED_PIXELS;
    m_FixedScreenInfo.visual            = FB_VISUAL_TRUECOLOR;

    m_VariableScreenInfo.xres           = m_Framebuffer.Width;
    m_VariableScreenInfo.yres           = m_Framebuffer.Height;
    m_VariableScreenInfo.xres_virtual   = m_Framebuffer.Width;
    m_VariableScreenInfo.yres_virtual   = m_Framebuffer.Height;
    m_VariableScreenInfo.bits_per_pixel = m_Framebuffer.BitsPerPixel;
    m_VariableScreenInfo.red            = {
                   .offset    = m_Framebuffer.RedMaskShift,
                   .length    = m_Framebuffer.RedMaskSize,
                   .msb_right = 0,
    };
    m_VariableScreenInfo.green = {
        .offset    = m_Framebuffer.GreenMaskShift,
        .length    = m_Framebuffer.GreenMaskSize,
        .msb_right = 0,
    };
    m_VariableScreenInfo.blue = {
        .offset    = m_Framebuffer.BlueMaskShift,
        .length    = m_Framebuffer.BlueMaskSize,
        .msb_right = 0,
    };

    m_VariableScreenInfo.activate = FB_ACTIVATE_NOW;
    m_VariableScreenInfo.vmode    = FB_VMODE_NONINTERLACED;
    m_VariableScreenInfo.width    = -1;
    m_VariableScreenInfo.height   = -1;

    DevTmpFs::RegisterDevice(this);

    StringView path = fmt::format("/dev/{}", GetName()).data();
    VFS::MkNod(VFS::GetRootNode(), path, 0666, GetID());
}

ErrorOr<isize> FramebufferDevice::Read(void* dest, off_t offset, usize bytes)
{
    if (bytes == 0) return 0;
    if (offset + bytes > m_FixedScreenInfo.smem_len)
        bytes = bytes - ((offset + bytes) - m_FixedScreenInfo.smem_len);

    std::memcpy(dest, m_Framebuffer.Address.Offset<void*>(offset), bytes);
    return bytes;
}
ErrorOr<isize> FramebufferDevice::Write(const void* src, off_t offset,
                                        usize bytes)
{
    if (bytes == 0) return 0;
    if (offset + bytes > m_FixedScreenInfo.smem_len)
        bytes = bytes - ((offset + bytes) - m_FixedScreenInfo.smem_len);

    std::memcpy(m_Framebuffer.Address.Offset<u8*>(offset), src, bytes);
    return bytes;
}

ErrorOr<isize> FramebufferDevice::Read(const UserBuffer& out, usize count,
                                       isize offset)
{
    return Read(out.Raw(), offset, count);
}
ErrorOr<isize> FramebufferDevice::Write(const UserBuffer& in, usize count,
                                        isize offset)
{
    return Write(in.Raw(), offset, count);
}

i32 FramebufferDevice::IoCtl(usize request, uintptr_t argp)
{
    switch (request)
    {
        case FBIOGET_VSCREENINFO:
            // TODO(v1tr10l7): should we validate those pointers here?
            std::memcpy(reinterpret_cast<u8*>(argp), &m_VariableScreenInfo,
                        sizeof(m_VariableScreenInfo));
            return 0;
        case FBIOGET_FSCREENINFO:
            // TODO(v1tr10l7): >>>>
            std::memcpy(reinterpret_cast<u8*>(argp), &m_FixedScreenInfo,
                        sizeof(m_FixedScreenInfo));
            return 0;
        case FBIOPUT_VSCREENINFO:
            m_VariableScreenInfo = *reinterpret_cast<fb_var_screeninfo*>(argp);
            return 0;
        case FBIOBLANK: return 0;
    }

    errno = ENOSYS;
    return -1;
}

bool FramebufferDevice::Initialize()
{
    usize framebufferCount = 0;
    auto  framebuffers     = BootInfo::GetFramebuffers(framebufferCount);

    if (framebufferCount == 0)
    {
        LogError("FbDev: Failed to acquire any framebuffers");
        return false;
    }

    if (s_PrimaryFramebuffer)
    {
        LogError(
            "FbDev: Using multiple framebuffer devices is currently not "
            "supported.");
        return false;
    };
    FramebufferDevice* primary = new FramebufferDevice(framebuffers[0]);
    s_PrimaryFramebuffer       = primary;

    auto path                  = fmt::format("/dev/{}", primary->GetName());
    LogTrace("FbDev: Successfully created framebuffer device at '{}'", path);

    return true;
}
