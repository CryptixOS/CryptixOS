/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/DeviceIDs.hpp>

#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/Terminal.hpp>
#include <Drivers/Video/FramebufferDevice.hpp>

#include <Prism/String/StringUtils.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

static FramebufferDevice*  s_PrimaryFramebuffer = nullptr;
static Atomic<DeviceMinor> s_NextFramebufferID  = 0;

FramebufferDevice::FramebufferDevice(StringView name, Framebuffer& framebuffer)
    : CharacterDevice(name, MakeDevice(ReserveMajor(), s_NextFramebufferID++))
{
    m_Framebuffer              = framebuffer;

    m_Stats.st_size            = 0;
    m_Stats.st_blocks          = 0;
    m_Stats.st_blksize         = 4096;
    m_Stats.st_rdev            = ID();
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
}

bool FramebufferDevice::Initialize()
{
    auto framebuffers = Terminal::Framebuffers();

    if (framebuffers.Empty())
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

    usize              index   = 0;
    auto               name    = "fb"_s + StringUtils::ToString(index);
    FramebufferDevice* primary = new FramebufferDevice(name, framebuffers[0]);
    if (!primary) return_err(false, ENOMEM);
    s_PrimaryFramebuffer = primary;

    auto path            = fmt::format("/dev/{}", primary->Name());
    LogTrace("FbDev: Successfully created framebuffer device at '{}'", path);

    auto result = DeviceManager::RegisterCharDevice(primary);
    if (!result)
    {
        delete primary;
        return false;
    }
    return true;
}
StringView     FramebufferDevice::Name() const noexcept { return m_Name; }

ErrorOr<isize> FramebufferDevice::Read(void* dest, off_t offset, usize bytes)
{
    if (bytes == 0) return 0;
    if (offset + bytes > m_FixedScreenInfo.smem_len)
        bytes = bytes - ((offset + bytes) - m_FixedScreenInfo.smem_len);

    Memory::Copy(dest, m_Framebuffer.Address.Offset<void*>(offset), bytes);
    return bytes;
}
ErrorOr<isize> FramebufferDevice::Write(const void* src, off_t offset,
                                        usize bytes)
{
    if (bytes == 0) return 0;
    if (offset + bytes > m_FixedScreenInfo.smem_len)
        bytes = bytes - ((offset + bytes) - m_FixedScreenInfo.smem_len);

    Memory::Copy(m_Framebuffer.Address.Offset<u8*>(offset), src, bytes);
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
            Memory::Copy(reinterpret_cast<u8*>(argp), &m_VariableScreenInfo,
                         sizeof(m_VariableScreenInfo));
            return 0;
        case FBIOGET_FSCREENINFO:
            // TODO(v1tr10l7): >>>>
            Memory::Copy(reinterpret_cast<u8*>(argp), &m_FixedScreenInfo,
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

DeviceMajor FramebufferDevice::ReserveMajor()
{
    static DeviceMajor major = []() -> DeviceMajor
    { return AllocateMajor(API::DeviceMajor::FRAMEBUFFER).Value(); }();

    return major;
}
