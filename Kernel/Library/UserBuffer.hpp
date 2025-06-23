/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/CPU.hpp>
#include <Arch/User.hpp>

#include <Prism/Memory/Pointer.hpp>

class [[nodiscard]] UserBuffer
{
  public:
    static ErrorOr<UserBuffer> ForUserBuffer(u8* userBuffer, usize size)
    {
        if (userBuffer && !Arch::InUserRange(userBuffer, size))
            return Error(EFAULT);

        return UserBuffer(userBuffer);
    }
    static ErrorOr<UserBuffer> ForUserBuffer(Pointer buffer, usize size)
    {
        if (buffer && !Arch::InUserRange(buffer, size)) return Error(EFAULT);

        return UserBuffer(buffer, size);
    }

    constexpr inline u8*   Raw() const { return m_Base; }
    constexpr inline usize Size() const { return m_Size; }

    template <typename T>
    inline ErrorOr<T> Read()
    {
        if (m_Size < sizeof(T)) return Error(EFAULT);

        CPU::UserMemoryProtectionGuard guard;
        T                              value;
        Read(&value, sizeof(T));

        return value;
    }
    template <typename T>
    inline ErrorOr<void> Write(const T& value)
    {
        if (m_Size < sizeof(T)) return Error(EFAULT);
        Write(&value, sizeof(T));
    }

    inline isize Read(Pointer destination, usize count, isize pos = -1)
    {
        if (pos < 0) pos = 0;
        usize                          copied = std::min(count, m_Size - pos);

        CPU::UserMemoryProtectionGuard guard;
        std::memcpy(destination.As<u8>(), m_Base.Offset<Pointer>(pos).As<u8>(),
                    count);

        return copied;
    }
    inline isize Write(Pointer source, usize count, isize pos = -1)
    {
        if (pos < 0) pos = 0;
        count = std::min(count, m_Size - pos);

        CPU::UserMemoryProtectionGuard guard;
        std::memcpy(m_Base.Offset<Pointer>(pos).As<u8>(), source.As<u8>(),
                    count);
        return count;
    }

  private:
    Pointer m_Base = nullptr;
    usize   m_Size = 0;

    explicit UserBuffer(Pointer buffer)
        : m_Base(buffer)
    {
    }
    explicit UserBuffer(Pointer buffer, usize size)
        : m_Base(buffer)
        , m_Size(size)
    {
    }
};
