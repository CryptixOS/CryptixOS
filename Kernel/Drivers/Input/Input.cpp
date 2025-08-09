/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/DeviceIDs.hpp>
#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/Input/Input.hpp>

#include <Library/Locking/SpinlockProtected.hpp>
#include <VFS/VFS.hpp>

static Spinlock                             s_Lock;
static Bitmap                               s_AllocatedMinors{};
static Atomic<DeviceMinor>                  s_LastFreeMinor = 0;

static UnorderedMap<DeviceID, InputDevice*> s_InputDevices;

InputDevice::InputDevice(StringView name, DeviceMinor minor)
    : CharacterDevice(name, API::DeviceMajor::INPUT, minor)
{
}

void InputDevice::Initialize()
{
    LogTrace("Input: Creating /dev/input...");
    Assert(VFS::CreateDirectory("/dev/input", 755));

    ScopedLock guard(s_Lock);
    Assert(DeviceManager::AllocateCharMajor(API::DeviceMajor::INPUT));
    s_AllocatedMinors.Allocate(4096);
}
DeviceMinor InputDevice::AllocateMinor()
{
    auto scanArea = [](const Bitmap& bitmap, usize start,
                       usize end) -> Optional<DeviceMinor>
    {
        for (usize i = start; i < end; i++)
            if (bitmap.GetIndex(i)) return i;

        return NullOpt;
    };

    ScopedLock guard(s_Lock);
    auto       minor = scanArea(s_AllocatedMinors, s_LastFreeMinor, 4096);
    if (!minor)
    {
        auto end        = s_LastFreeMinor.Load();
        s_LastFreeMinor = 0;
        minor           = scanArea(s_AllocatedMinors, 0, end);
    }

    if (minor)
    {
        s_AllocatedMinors.SetIndex(*minor, true);
        ++s_LastFreeMinor;
        if (s_LastFreeMinor >= 4095) s_LastFreeMinor = 0;

        return minor;
    }

    return minor;
}
void InputDevice::FreeMinor(DeviceMinor minor)
{
    ScopedLock guard(s_Lock);
    s_AllocatedMinors.SetIndex(minor, false);
}

void InputDevice::Register(InputDevice* input)
{
    Assert(input);

    ScopedLock guard(s_Lock);
    auto       id      = input->ID();

    s_InputDevices[id] = input;
}
void InputDevice::Unregister(InputDevice* input)
{
    Assert(input);

    ScopedLock guard(s_Lock);
    auto       id = input->ID();

    s_InputDevices.Erase(id);
}
