/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/DeviceIDs.hpp>

#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/TTY/VirtualConsole.hpp>
#include <Drivers/Video/FramebufferDevice.hpp>

#include <Prism/Algorithm/Find.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Compare.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

static Mutex               s_Mutex;
static FramebufferDevice*  s_PrimaryFramebuffer = nullptr;
static Atomic<DeviceMinor> s_NextFramebufferID  = 0;
Vector<FramebufferDevice*> FramebufferDevice::s_Framebuffers;

inline constexpr usize     FBPIXMAPSIZE      = 1024 << 3;
inline constexpr usize     FB_PIXMAP_DEFAULT = 1;

FramebufferDevice::FramebufferDevice(StringView name, Framebuffer& framebuffer)
    : CharacterDevice(name, MakeDevice(API::DeviceMajor::FRAMEBUFFER,
                                       s_NextFramebufferID++))
{
    m_Framebuffer              = framebuffer;

    m_Stats.st_size            = 0;
    m_Stats.st_blocks          = 0;
    m_Stats.st_blksize         = PMM::PAGE_SIZE;
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
    if (!DeviceManager::AllocateCharMajor(API::DeviceMajor::FRAMEBUFFER))
    {
        LogError("unable to get major %d for fb devs\n",
                 API::DeviceMajor::FRAMEBUFFER);
        return false;
    }
    auto framebuffers = VirtualConsole::Framebuffers();

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

ErrorOr<void> FramebufferDevice::Register(FramebufferDevice& framebuffer)
{
    s_Mutex.Lock();

    if (framebuffer.CheckForeignness()) return Error(ENOSYS);

    if (s_Framebuffers.Size() >= FB_MAX) return Error(ENXIO);
    framebuffer.m_Count = 1;

    if (framebuffer.m_Pixmap.addr == NULL)
    {
        framebuffer.m_Pixmap.addr = new u8[FBPIXMAPSIZE];
        if (framebuffer.m_Pixmap.addr)
        {
            framebuffer.m_Pixmap.size         = FBPIXMAPSIZE;
            framebuffer.m_Pixmap.buf_align    = 1;
            framebuffer.m_Pixmap.scan_align   = 1;
            framebuffer.m_Pixmap.access_align = 32;
            framebuffer.m_Pixmap.flags        = FB_PIXMAP_DEFAULT;
        }
    }
    framebuffer.m_Pixmap.offset = 0;

    if (!framebuffer.m_Pixmap.blit_x) framebuffer.m_Pixmap.blit_x = ~(u32)0;
    if (!framebuffer.m_Pixmap.blit_y) framebuffer.m_Pixmap.blit_y = ~(u32)0;

    s_Mutex.Unlock();
    return {};
}

StringView     FramebufferDevice::Name() const noexcept { return m_Name; }

ErrorOr<isize> FramebufferDevice::Read(void* dest, off_t offset, usize bytes)
{
    unsigned long p = offset;
    u8*           buffer;
    i32           c, cnt = 0, err = 0;

    if (!m_ScreenBase) return Error(ENODEV);

    // if (m_State != FBINFO_STATE_RUNNING) return Error(EPERM);

    usize totalSize = m_ScreenSize;

    if (totalSize == 0) totalSize = m_FixedScreenInfo.smem_len;

    if (p >= totalSize) return 0;

    if (bytes >= totalSize) bytes = totalSize;
    if (bytes + p > totalSize) bytes = totalSize - p;

    buffer = new u8[bytes > PMM::PAGE_SIZE ? PMM::PAGE_SIZE : bytes];
    if (!buffer) return Error(ENOMEM);

    u8* src = reinterpret_cast<u8*>(m_ScreenBase + p);

    u8* dst = 0;
    while (bytes)
    {
        c   = (bytes > PMM::PAGE_SIZE) ? PMM::PAGE_SIZE : bytes;
        dst = buffer;
        Memory::Copy(dst, src, c);
        dst += c;
        src += c;

        if (CopyToUser(dest, buffer, c)) return Error(EFAULT);
        offset += c;
        dest = reinterpret_cast<u8*>(dest) + c;
        cnt += c;
        bytes -= c;
    }

    delete[] buffer;
    return err ? err : cnt;
}
ErrorOr<isize> FramebufferDevice::Write(const void* source, off_t offset,
                                        usize bytes)
{
    usize p = offset;
    u8*   buffer;
    int   c, cnt = 0, err = 0;

    if (!m_ScreenBase) return Error(ENODEV);

    // if (m_State != FBINFO_STATE_RUNNING) return Error(EPERM);

    usize totalSize = m_ScreenSize;
    if (totalSize == 0) totalSize = m_FixedScreenInfo.smem_len;

    if (p > totalSize) return Error(EFBIG);
    if (bytes > totalSize)
    {
        err   = -EFBIG;
        bytes = totalSize;
    }

    if (bytes + p > totalSize)
    {
        if (!err) err = -ENOSPC;

        bytes = totalSize - p;
    }

    buffer = new u8[bytes > PMM::PAGE_SIZE ? PMM::PAGE_SIZE : bytes];
    if (!buffer) return Error(ENOMEM);

    u8* dst = reinterpret_cast<u8*>(m_ScreenBase + p);

    u8* src = nullptr;
    while (bytes)
    {
        c   = (bytes > PMM::PAGE_SIZE) ? PMM::PAGE_SIZE : bytes;
        src = buffer;

        if (CopyFromUser(src, source, c))
        {
            err = -EFAULT;
            break;
        }

        Memory::Copy(dst, src, c);
        dst += c;
        src += c;
        offset += c;
        source = reinterpret_cast<const u8*>(source) + c;
        cnt += c;
        bytes -= c;
    }

    delete[] buffer;

    return cnt ? cnt : err;
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

ErrorOr<isize> FramebufferDevice::IoCtl(usize request, upointer argp)
{
    switch (request)
    {
        case FBIOGET_VSCREENINFO:
            CopyToUser(argp, &m_VariableScreenInfo,
                       sizeof(m_VariableScreenInfo));
            break;
        case FBIOPUT_VSCREENINFO:
        {
            fb_var_screeninfo var{};
            if (auto result
                = CopyFromUser(&var, argp, sizeof(m_VariableScreenInfo));
                !result)
                return Error(result.Error());
            m_Flags |= FBINFO_MISC_USEREVENT;
            auto ret = SetVar(var);
            m_Flags &= ~FBINFO_MISC_USEREVENT;
            if (!ret
                && CopyToUser(argp, &m_VariableScreenInfo,
                              sizeof(m_VariableScreenInfo)))
                return Error(EFAULT);
            break;
        }
        case FBIOGET_FSCREENINFO:
        {
            if (!m_Lock.TryLock()) return Error(ENODEV);
            fb_fix_screeninfo fix = m_FixedScreenInfo;
            m_Lock.Unlock();
            if (auto result = CopyToUser(argp, &fix, sizeof(fix)); !result)
                return Error(result.Error());
            return {};
        }
        case FBIOPUTCMAP:
        {
            fb_cmap cmap;
            if (auto result = CopyFromUser(&cmap, argp, sizeof(cmap)); !result)
                return Error(result.Error());

            auto result = SetUserColorMap(cmap);
            if (!result) return Error(result.Error());
            return {};
        }
    }

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

    return Error(ENOSYS);
}

u32 FramebufferDevice::GetColorDepth()
{
    int depth = 0;

    if (m_FixedScreenInfo.visual == FB_VISUAL_MONO01
        || m_FixedScreenInfo.visual == FB_VISUAL_MONO10)
        depth = 1;
    else {
        if (m_VariableScreenInfo.green.length
                == m_VariableScreenInfo.blue.length
            && m_VariableScreenInfo.green.length
                   == m_VariableScreenInfo.red.length
            && m_VariableScreenInfo.green.offset
                   == m_VariableScreenInfo.blue.offset
            && m_VariableScreenInfo.green.offset
                   == m_VariableScreenInfo.red.offset)
            depth = m_VariableScreenInfo.green.length;
        else
            depth = m_VariableScreenInfo.green.length
                  + m_VariableScreenInfo.red.length
                  + m_VariableScreenInfo.blue.length;
    }

    return depth;
}

ErrorOr<void> FramebufferDevice::RemoveVideoMode(const fb_videomode& mode)
{
    auto found = Find(m_VideoModes.begin(), m_VideoModes.end(), mode);
    if (found == m_VideoModes.end()) return Error(EINVAL);

    m_VideoModes.Erase(found);
    return {};
}
ErrorOr<void> FramebufferDevice::SetVar(fb_var_screeninfo& var)
{
    if (var.activate & FB_ACTIVATE_INV_MODE)
    {
        fb_videomode mode1 = m_VariableScreenInfo.VideoMode();
        fb_videomode mode2 = var.VideoMode();

        /* make sure we don't delete the videomode of current var */
        bool         equal = mode1 == mode2;

        if (equal) { return {}; }
        if (auto result = RemoveVideoMode(mode1); !result)
            return Error(result.Error());

        return {};
    }

    if ((var.activate & FB_ACTIVATE_FORCE)
        || Memory::Compare(&m_VariableScreenInfo, &var,
                           sizeof(struct fb_var_screeninfo)))
    {
        u32  activate = var.activate;

        auto ret      = CheckVariableScreenInfo(var);
        if (!ret && ret.Error() == ENOSYS)
        {
            var = m_VariableScreenInfo;
            return {};
        }
        if (ret) return {};

        if ((var.activate & FB_ACTIVATE_MASK) == FB_ACTIVATE_NOW)
        {
            struct fb_var_screeninfo oldVar;
            auto                     ret = CheckCapabilities(var, activate);
            if (ret) return {};

            oldVar               = m_VariableScreenInfo;
            m_VariableScreenInfo = var;
        }
    }

    return {};
}
ErrorOr<void> FramebufferDevice::SetUserColorMap(fb_cmap& cmap)
{
    int            size = cmap.len * sizeof(u16);
    struct fb_cmap umap;

    if (size < 0 || size < static_cast<int>(cmap.len)) return Error(E2BIG);

    Memory::Fill(&umap, 0, sizeof(struct fb_cmap));

    if (!CopyFromUser(umap.red, cmap.red, size)
        || !CopyFromUser(umap.green, cmap.green, size)
        || !CopyFromUser(umap.blue, cmap.blue, size)
        || (cmap.transp && !CopyFromUser(umap.transp, cmap.transp, size)))
    {
        // TODO(v1tr10l7): Free CMAP
        return Error(EFAULT);
    }
    umap.start = cmap.start;
    if (!m_Lock.TryLock())
    {
        // TODO(v1tr10l7): Free CMAP
        return Error(ENODEV);
    }
    m_Lock.Unlock();
    return {};
}

ErrorOr<void> FramebufferDevice::CheckForeignness() { return {}; }
