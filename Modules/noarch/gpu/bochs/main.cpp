/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Prism/Core/Types.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
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
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <Prism/Containers/Array.hpp>

import Cryptix;

class BochsAdapter : public PCI::Device
{
  public:
    explicit BochsAdapter(PCI::DeviceAddress address)
        : Device(address)
    {
    }

    enum class VbeRegister
    {
        eID            = 0,
        eResolutionX   = 1,
        eResolutionY   = 2,
        eBitsPerPixel  = 3,
        eEnable        = 4,
        eBank          = 5,
        eVirtualWidth  = 6,
        eVirtualHeight = 7,
        eOffsetX       = 8,
        eOffsetY       = 9,
    };

    static ErrorOr<void> Probe(PCI::DeviceAddress&  address,
                               const PCI::DeviceID& id)
    {
        u16 vbeID = Read(VbeRegister::eID);
        if (vbeID < 0xb0c0 || vbeID > 0xb0c5) return Error(ENODEV);

        // BochsAdapter* bxvga = new BochsAdapter(address);
        return {};
    }

  private:
    constexpr static u16 VBE_DISPI_IOPORT_INDEX = 0x01ce;
    constexpr static u16 VBE_DISPI_IOPORT_DATA  = 0x01cf;

    inline static u16    Read(VbeRegister index)
    {
        IO::Out<word>(VBE_DISPI_IOPORT_INDEX, ToUnderlying(index));
        return IO::In<word>(VBE_DISPI_IOPORT_DATA);
    }
    inline static void Write(VbeRegister index, u16 value)
    {
        IO::Out<word>(VBE_DISPI_IOPORT_INDEX, ToUnderlying(index));
        IO::Out<word>(VBE_DISPI_IOPORT_DATA, value);
    }
};

static Array       s_BochsIdTable = ToArray({
    PCI::DeviceID{0x1234, 0x1111, PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID,
                  PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID},
    PCI::DeviceID{0x4321, 0x1111, PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID,
                  PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID},
});
static PCI::Driver s_BochsDriver  = {
     .Name     = "bxvga",
     .MatchIDs = Span(s_BochsIdTable.begin(), s_BochsIdTable.Size()),
     .Probe    = BochsAdapter::Probe,
     .Remove   = nullptr,
};

static bool ModuleInit() { return PCI::RegisterDriver(s_BochsDriver); }
MODULE_INIT(bxvga, ModuleInit);
