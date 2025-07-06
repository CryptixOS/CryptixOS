/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#ifdef CTOS_TARGET_X86_64
    #include <Arch/x86_64/IO.hpp>
#endif

#include <Memory/AddressRange.hpp>
#include <Memory/MMIO.hpp>

enum class IoSpace
{
    eNone = 0x00,
    eMMIO = 0x01,
#ifdef CTOS_TARGET_X86_64
    eSystemIO = 0x02,
#endif
};

struct SystemIO_Tag
{
};
class IoRegion
{
  public:
    constexpr IoRegion() = default;

    PM_ALWAYS_INLINE constexpr IoRegion(Pointer m_Base, usize size)
        : m_MemoryMappedIO(m_Base, size)
        , m_IoSpace(IoSpace::eMMIO)
    {
    }
#ifdef CTOS_TARGET_X86_64
    PM_ALWAYS_INLINE constexpr IoRegion(u16 firstPort, u16 portCount,
                                        SystemIO_Tag)
        : m_IoSpace(IoSpace::eSystemIO)
        , m_SystemIO(firstPort, portCount)
    {
    }
#endif

    void Initialize(usize start, usize count, IoSpace ioSpace)
    {
        m_IoSpace = ioSpace;
#ifdef CTOS_TARGET_X86_64
        if (m_IoSpace == IoSpace::eSystemIO)
            m_SystemIO = {start, count};
        else
#else
            Assert("SystemIO is not supported on this platform");
#endif
            m_MemoryMappedIO = {start, count};
    }

    template <UnsignedIntegral T>
    PM_ALWAYS_INLINE constexpr T Read(usize offset) const
    {
#ifdef CTOS_TARGET_X86_64
        if (m_IoSpace == IoSpace::eSystemIO)
        {
            Assert(sizeof(T) <= 4 && offset < m_SystemIO.Size());
            return IO::In<T>(m_SystemIO.Base().Offset(offset));
        }
        else
#else
        Assert(offset < m_MemoryMappedIO.Size());
#endif
            return MMIO::Read<T>(m_MemoryMappedIO.Base().Offset(offset));
    }
    template <UnsignedIntegral T>
    PM_ALWAYS_INLINE constexpr void Write(usize offset, T value) const
    {
#ifdef CTOS_TARGET_X86_64
        if (m_IoSpace == IoSpace::eSystemIO)
        {
            Assert(sizeof(T) <= 4 && offset < m_SystemIO.Size());
            IO::Out<T>(m_SystemIO.Base().Offset(offset), value);
        }
        else
#else
        Assert(offset < m_MemoryMappedIO.Size());
#endif
            MMIO::Write<T>(m_MemoryMappedIO.Base().Offset(offset), value);
    }

  private:
    AddressRange m_MemoryMappedIO;
    IoSpace      m_IoSpace = IoSpace::eMMIO;

#ifdef CTOS_TARGET_X86_64
    AddressRange m_SystemIO;
#endif
};
