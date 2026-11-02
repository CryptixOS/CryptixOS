/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Prism/Core/Types.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
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
#include <Prism/Core/Concepts.hpp>
#include <API/UnixTypes.hpp>
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
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

import Cryptix;

CTOS_MODULE_AUTHOR("v1tr10l7");
CTOS_MODULE_DESCRIPTION("pc speaker driver");
CTOS_MODULE_LICENSE("GPL-3");
CTOS_MODULE_VERSION("0.2");

class PCSpeakerDevice : public CharacterDevice
{
  public:
    PCSpeakerDevice()
        : CharacterDevice("pcspk", API::DeviceMajor::MISCELLANEOUS, 254)
    {
    }

    virtual StringView     Name() const noexcept override { return m_Name; }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override
    {
        if (!src || bytes < sizeof(u32)) return Error(EINVAL);

        struct Command
        {
            u16 Tone         = 0;
            u16 Milliseconds = 0;
        } command = AsUser([src]() -> Command
                           { return *reinterpret_cast<const Command*>(src); });

        if (command.Tone < 20 || command.Tone > 20'000
            || command.Milliseconds == 0)
            return Error(EINVAL);

        PCSpeaker::ToneOn(command.Tone);
        IO::Delay(command.Milliseconds);
        PCSpeaker::ToneOff();

        return sizeof(Command);
    }

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        return Read(out.Raw(), offset, count);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        return Write(in.Raw(), offset, count);
    }

    virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override
    {
        return Error(ENOSYS);
    }
};

namespace
{
    static PCSpeakerDevice* s_PCSpeaker = nullptr;
    ErrorOr<void>           Probe()
    {
        s_PCSpeaker = new PCSpeakerDevice();
        if (!s_PCSpeaker) return Error(ENOMEM);

        RetOnError(DeviceManager::RegisterCharDevice(s_PCSpeaker));
        return {};
    }

    static GenericDriver s_PCSpeakerDriver = {
        .Name   = "pcspk",
        .Probe  = Probe,
        .Remove = nullptr,
    };

    extern "C" CTOS_EXPORT bool ModuleInit()
    {
        return RegisterGenericDriver(s_PCSpeakerDriver);
    }
} // namespace
MODULE_INIT(pcspk, ModuleInit);
