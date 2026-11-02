/*
 * Created by v1tr10l7 on 11.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

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
#include <Prism/Memory/Scope.hpp>

export module Cryptix.Modules.AtKeyboard;

import Cryptix;
import Cryptix.Modules.Serio;
export import :Scancodes;

export enum class KeyModifier
{
    eNone     = 0x00,
    eControl  = Bit(0),
    eAlt      = Bit(1),
    eShift    = Bit(2),
    eCapsLock = Bit(3),
};

export constexpr inline KeyModifier operator~(KeyModifier lhs)
{
    auto result = ~ToUnderlying(lhs);
    return static_cast<KeyModifier>(result);
}
export constexpr inline bool operator&(KeyModifier lhs, KeyModifier rhs)
{
    return ToUnderlying(lhs) & ToUnderlying(rhs);
}
export constexpr inline KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs)
{
    u64 result = ToUnderlying(lhs) | ToUnderlying(rhs);

    return (lhs = static_cast<KeyModifier>(result));
}
export constexpr inline KeyModifier& operator&=(KeyModifier& lhs, KeyModifier rhs)
{
    u64 result = ToUnderlying(lhs) & ToUnderlying(rhs);

    return (lhs = static_cast<KeyModifier>(result));
}

export class AtKeyboard : public SerioDevice
{
  public:
    enum class ScanCodeSet
    {
        eSet1,
        eSet2,
        eSet3,
    };

    AtKeyboard(SerioController* ctrl, SerioDevicePort port,
               ScanCodeSet scanCodeSet)
        : SerioDevice("atkbd", ctrl)
        , m_Controller(ctrl)
        , m_Port(port)
        , m_ScanCodeSet(scanCodeSet)
    {
        Initialize();
    }
    virtual ~AtKeyboard() = default;

    void                   Initialize();
    virtual StringView     Name() const noexcept override { return m_Name; }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        // TODO(v1tr10l7): Read from keyboard
        return -1;
    };
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override
    {
        // TODO(v1tr10l7): Write to keyboard
        return -1;
    }

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        return -1;
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        return -1;
    }

    virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override
    {
        // TODO(v1tr10l7): AtKeyboard::IoCtl
        return Error(ENOSYS);
    }

    virtual void OnByteReceived(u8 byte) override;

  private:
    SerioController* m_Controller = nullptr;
    SerioDevicePort  m_Port;
    ScanCodeSet      m_ScanCodeSet   = ScanCodeSet::eSet1;

    KeyModifier      m_Modifiers     = KeyModifier::eNone;
    bool             m_ExtraScanCode = false;

    void             HandleScanCodeSet1Key(u8 raw);
    void             HandleScanCodeSet2Key(u8 raw);

    void             Emit(const char* str, usize count);
};

export using Ps2ScanCodeSet = AtKeyboard::ScanCodeSet;
