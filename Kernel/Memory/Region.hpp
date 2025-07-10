/*
 * Created by v1tr10l7 on 18.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/Posix/sys/mman.h>
#include <Memory/AddressRange.hpp>
#include <Prism/Memory/Ref.hpp>

class FileDescriptor;
enum class PageAttributes : isize;

namespace VMM
{
    enum class Access : u8
    {
        eNone             = 0x00,
        eRead             = 0x01,
        eWrite            = 0x02,
        eExecute          = 0x04,
        eUser             = 0x08,
        eReadOnly         = eRead,
        eReadWrite        = eRead | eWrite,
        eReadWriteExecute = eReadWrite | eExecute,
    };
    inline constexpr Access& operator|=(Access& lhs, const Access rhs)
    {
        auto result = ToUnderlying(lhs) | ToUnderlying(rhs);
        lhs         = static_cast<Access>(result);

        return lhs;
    }

    constexpr bool operator&(const Access lhs, const Access rhs)
    {
        return ToUnderlying(lhs) & ToUnderlying(rhs);
    }
    constexpr Access operator|(const Access lhs, const Access rhs)
    {
        auto result = ToUnderlying(lhs) | ToUnderlying(rhs);

        return static_cast<Access>(result);
    }

    class Region final : public RefCounted
    {
      public:
        Region() = default;
        Region(Pointer phys, Pointer virt, usize size,
               FileDescriptor* fd = nullptr)
            : m_VirtualRange(virt, size)
            , m_PhysicalBase(phys)
            , m_Fd(fd)

        {
        }

        inline bool Contains(const Pointer address) const
        {
            return m_VirtualRange.Contains(address);
        }

        inline const AddressRange& VirtualRange() const
        {
            return m_VirtualRange;
        }
        inline Pointer PhysicalBase() const { return m_PhysicalBase; }
        inline Pointer VirtualBase() const { return m_VirtualRange.Base(); }
        inline Pointer End() const
        {
            return m_VirtualRange.Base().Offset(m_VirtualRange.Size());
        }

        inline usize           Size() const { return m_VirtualRange.Size(); }
        inline FileDescriptor* FileDescriptor() const { return m_Fd; }

        inline enum Access     Access() const { return m_Access; }
        PageAttributes         PageAttributes() const;

        inline void    SetPhysicalBase(Pointer phys) { m_PhysicalBase = phys; }
        inline void    SetAccessMode(enum Access access) { m_Access = access; }

        constexpr bool IsReadable() const { return m_Access & Access::eRead; }
        constexpr bool IsWriteable() const { return m_Access & Access::eWrite; }
        constexpr bool IsExecutable() const
        {
            return m_Access & Access::eExecute;
        }

      private:
        AddressRange          m_VirtualRange;
        Pointer               m_PhysicalBase = nullptr;
        enum Access           m_Access       = Access::eNone;
        class FileDescriptor* m_Fd           = nullptr;
    };
}; // namespace VMM
using VMM::Region;
