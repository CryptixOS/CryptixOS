/*
 * Created by v1tr10l7 on 31.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/KernelMessageDevice.hpp>

KernelMessageDevice* KernelMessageDevice::s_Instance = nullptr;

isize                KernelMessageDevice::Write(StringView str)
{
    if (!s_Instance) return -1;

    auto result = s_Instance->Write(str.Raw(), 0, str.Size());
    if (!result) return -1;
    return *result;
}

KernelMessageDevice::KernelMessageDevice()
    : CharacterDevice("kmsg", MakeDevice(1, 11))
{
}
KernelMessageDevice::~KernelMessageDevice() {}

ErrorOr<isize> KernelMessageDevice::Read(void* dest, off_t offset, usize bytes)
{
    return m_Buffer.Read(static_cast<u8*>(dest), bytes);
}
ErrorOr<isize> KernelMessageDevice::Write(const void* src, off_t offset,
                                          usize bytes)
{
    return m_Buffer.Write(static_cast<const u8*>(src), bytes);
}
