/*
 * Created by v1tr10l7 on 27.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/CharacterDevice.hpp>
#include <Drivers/HID/Ps2Controller.hpp>

#include <Prism/Memory/Scope.hpp>
#include <Prism/Memory/WeakRef.hpp>

enum class KeyModifier
{
    eNone     = 0x00,
    eControl  = Bit(0),
    eAlt      = Bit(1),
    eShift    = Bit(2),
    eCapsLock = Bit(3),
};

constexpr inline KeyModifier operator~(KeyModifier lhs)
{
    auto result = ~ToUnderlying(lhs);
    return static_cast<KeyModifier>(result);
}
constexpr inline bool operator&(KeyModifier lhs, KeyModifier rhs)
{
    return ToUnderlying(lhs) & ToUnderlying(rhs);
}
constexpr inline KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs)
{
    u64 result = ToUnderlying(lhs) | ToUnderlying(rhs);

    return (lhs = static_cast<KeyModifier>(result));
}
constexpr inline KeyModifier& operator&=(KeyModifier& lhs, KeyModifier rhs)
{
    u64 result = ToUnderlying(lhs) & ToUnderlying(rhs);

    return (lhs = static_cast<KeyModifier>(result));
}

class Ps2KeyboardDevice : public RefCounted, public CharacterDevice
{
  public:
    enum class ScanCodeSet
    {
        eSet1,
        eSet2,
        eSet3,
    };

    Ps2KeyboardDevice(Ps2Controller* controller, PS2_DevicePort port,
                      ScanCodeSet scanCodeSet)
        : CharacterDevice("atkbd", MakeDevice(AllocateMajor().Value(), 0))
        , m_Controller(controller)
        , m_Port(port)
        , m_ScanCodeSet(scanCodeSet)
    {
        Initialize();
    }
    virtual ~Ps2KeyboardDevice() = default;

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

    virtual i32 IoCtl(usize request, uintptr_t argp) override
    {
        // TODO(v1tr10l7): Ps2KeyboardDevice::IoCtl
        return -1;
    }

    void OnByteReceived(u8 byte);

  private:
    Ps2Controller* m_Controller = nullptr;
    PS2_DevicePort m_Port;
    ScanCodeSet    m_ScanCodeSet   = ScanCodeSet::eSet1;

    KeyModifier    m_Modifiers     = KeyModifier::eNone;
    bool           m_ExtraScanCode = false;

    void           HandleScanCodeSet1Key(u8 raw);
    void           HandleScanCodeSet2Key(u8 raw);

    void           Emit(const char* str, usize count);
};

using Ps2ScanCodeSet = Ps2KeyboardDevice::ScanCodeSet;
