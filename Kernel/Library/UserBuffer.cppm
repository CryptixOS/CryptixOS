/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
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

export module Cryptix:Library.UserBuffer;

export import :Arch.CPU;
export import :Arch.User;

export class [[nodiscard]] UserBuffer
{
  public:
    static ErrorOr<UserBuffer> ForUserBuffer(u8* userBuffer, usize size)
    {
        if (userBuffer && !InUserRange(userBuffer, size)) return Error(EFAULT);

        return UserBuffer(userBuffer);
    }
    static ErrorOr<UserBuffer> ForUserBuffer(Pointer buffer, usize size)
    {
        if (buffer && !InUserRange(buffer, size)) return Error(EFAULT);

        return UserBuffer(buffer, size);
    }

    constexpr inline u8*   Raw() const { return m_Base; }
    constexpr inline usize Size() const { return m_Size; }

    template <typename T>
    inline ErrorOr<T> Read()
    {
        if (m_Size < sizeof(T)) return Error(EFAULT);

        UserMemoryProtectionGuard guard;
        T                         value;
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
        usize                     copied = Min(count, m_Size - pos);

        UserMemoryProtectionGuard guard;
        Memory::Copy(destination.As<u8>(), m_Base.Offset<Pointer>(pos).As<u8>(),
                     count);

        return copied;
    }
    inline isize Write(Pointer source, usize count, isize pos = -1)
    {
        if (pos < 0) pos = 0;
        count = Min(count, m_Size - pos);

        UserMemoryProtectionGuard guard;
        Memory::Copy(m_Base.Offset<Pointer>(pos).As<u8>(), source.As<u8>(),
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
