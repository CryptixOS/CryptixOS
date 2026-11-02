/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/linux/fb.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>

export module Cryptix:Drivers.Video.FramebufferDevice;

export import :Drivers.Core.CharacterDevice;
export import :Drivers.Video.Framebuffer;
export import :Library.Locking.Mutex;

export class FramebufferDevice : public CharacterDevice
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
