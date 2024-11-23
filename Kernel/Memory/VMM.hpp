/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/BootInfo.hpp"

template <typename Address>
inline static constexpr bool IsHigherHalfAddress(Address addr)
{
    return reinterpret_cast<uintptr_t>(addr) >= BootInfo::GetHHDMOffset();
}

template <typename T, typename U>
inline static constexpr T ToHigherHalfAddress(U addr)
{
    T ret = IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(reinterpret_cast<uintptr_t>(addr)
                                    + BootInfo::GetHHDMOffset());
    return ret;
}

template <typename T, typename U>
inline static constexpr T FromHigherHalfAddress(U addr)
{
    T ret = !IsHigherHalfAddress(addr)
              ? reinterpret_cast<T>(addr)
              : reinterpret_cast<T>(reinterpret_cast<uintptr_t>(addr)
                                    - BootInfo::GetHHDMOffset());
    return ret;
}
