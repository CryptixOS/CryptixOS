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

class FileDescriptor;
enum class PageAttributes : isize;

namespace VirtualMemoryManager
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
        auto result = std::to_underlying(lhs) | std::to_underlying(rhs);
        lhs         = static_cast<Access>(result);

        return lhs;
    }

    constexpr bool operator&(const Access lhs, const Access rhs)
    {
        return std::to_underlying(lhs) & std::to_underlying(rhs);
    }
    constexpr Access operator|(const Access lhs, const Access rhs)
    {
        auto result = std::to_underlying(lhs) | std::to_underlying(rhs);

        return static_cast<Access>(result);
    }

    class Region final
    {
      public:
        Region() = default;
        Region(Pointer phys, Pointer virt, usize size,
               i32 flags = PROT_READ | PROT_WRITE, FileDescriptor* fd = nullptr)
            : m_VirtualRange(virt, size)
            , m_PhysicalBase(phys)
            , m_Flags(flags)
            , m_Fd(fd)

        {
        }

        inline bool Contains(const Pointer address) const
        {
            return m_VirtualRange.Contains(address);
        }

        inline const AddressRange& GetVirtualRange() const
        {
            return m_VirtualRange;
        }
        inline Pointer GetPhysicalBase() const { return m_PhysicalBase; }
        inline Pointer GetVirtualBase() const
        {
            return m_VirtualRange.GetBase();
        }
        inline Pointer End() const
        {
            return m_VirtualRange.GetBase().Offset(m_VirtualRange.GetSize());
        }

        inline usize GetSize() const { return m_VirtualRange.GetSize(); }
        inline FileDescriptor* GetFileDescriptor() const { return m_Fd; }

        inline Access          GetAccess() const { return m_Access; }
        inline i32             GetProt() const { return m_Flags; }
        PageAttributes         GetPageAttributes() const;

        inline void SetPhysicalBase(Pointer phys) { m_PhysicalBase = phys; }
        inline void SetProt(Access access, i32 prot)
        {
            m_Access = access;
            m_Flags  = prot;
        }

        constexpr bool ValidateFlags(i32 flags) const
        {
            return m_Flags & flags;
        }
        constexpr bool IsReadable() const { return m_Access & Access::eRead; }
        constexpr bool IsWriteable() const { return m_Access & Access::eWrite; }
        constexpr bool IsExecutable() const
        {
            return m_Access & Access::eExecute;
        }

      private:
        AddressRange    m_VirtualRange;
        Pointer         m_PhysicalBase = nullptr;
        Access          m_Access       = Access::eNone;
        i32             m_Flags        = 0;
        FileDescriptor* m_Fd           = nullptr;
    };
}; // namespace VirtualMemoryManager
using VirtualMemoryManager::Region;
