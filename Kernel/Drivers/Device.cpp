/*
 * Created by v1tr10l7 on 25.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Device.hpp>

Optional<DeviceMajor> Device::s_LeastMajor = 0;
Bitmap                Device::s_AllocatedMajors{};

void                  Device::Initialize() { s_AllocatedMajors.Allocate(4096); }
Optional<DeviceMajor> Device::AllocateMajor(usize hint)
{
    Optional<DeviceMajor> allocated = FindFreeMajor(hint);
    if (!allocated) return NullOpt;

    if (allocated == s_LeastMajor) s_LeastMajor = FindFreeMajor(0);
    s_AllocatedMajors.SetIndex(allocated.Value(), true);
    return allocated;
}

Optional<DeviceMajor> Device::FindFreeMajor(isize start, isize end)
{
    Optional<DeviceMajor> allocated = NullOpt;

    for (isize i = start; i < end; i++)
        if (!s_AllocatedMajors.GetIndex(i)) allocated = i;

    return allocated;
}
Optional<DeviceMajor> Device::FindFreeMajor(DeviceMajor hint)
{
    usize                 last  = s_AllocatedMajors.GetSize();
    Optional<DeviceMajor> major = FindFreeMajor(hint, last);
    if (!major) major = FindFreeMajor(0, hint);

    return major;
}
