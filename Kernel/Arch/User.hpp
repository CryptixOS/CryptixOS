/*
 * Created by v1tr10l7 on 30.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Utility/Path.hpp>

constexpr inline bool InUserRange(Pointer virt, usize bytes)
{
    return !virt.IsHigherHalf();
}

ErrorOr<void> CopyToUser(Pointer destination, Pointer source, usize count);
ErrorOr<void> CopyFromUser(Pointer destination, Pointer source, usize count);
ErrorOr<void> FillUser(Pointer buffer, isize value, usize count);

// NOTE(v1tr10l7): allows accessing usermode memory from ring0
struct UserMemoryProtectionGuard
{
    UserMemoryProtectionGuard();
    ~UserMemoryProtectionGuard();
};

template <typename F, typename... Args>
    requires(!SameAs<Prism::InvokeResultType<F, Args...>, void>)
inline decltype(auto) AsUser(F&& f, Args&&... args)
{
    UserMemoryProtectionGuard guard;

    return f(Forward<Args>(args)...);
}
template <typename T>
    requires(!SameAs<T, void>)
inline RemoveReferenceType<T> CopyFromUser(const T& value)
{
    UserMemoryProtectionGuard guard;

    return value;
}
template <typename T>
    requires(!SameAs<T, void>)
inline RemoveReferenceType<T> CopyFromUser(const T& value, usize size)
{
    T                         copied = {};
    UserMemoryProtectionGuard guard;
    Memory::Copy(&copied, &value, size);

    return copied;
}
inline Path CopyStringFromUser(const char* string)
{
    return AsUser([string]() -> Path { return string; });
}

template <typename T>
    requires(!SameAs<T, void>)
inline void CopyToUser(T* userBuffer, const T& value)
{
    UserMemoryProtectionGuard guard;

    *userBuffer = value;
}
inline void CopyStringToUser(StringView source, char* dest, isize count)
{
    UserMemoryProtectionGuard guard;

    if (count < 0) count = source.Size();
    source.Copy(dest, count);
}

template <typename F, typename... Args>
inline static void AsUser(F&& f, Args&&... args)
{
    UserMemoryProtectionGuard guard;
    f(Forward<Args>(args)...);
}
