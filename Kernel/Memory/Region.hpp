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
namespace VirtualMemoryManager
{
    class Region final
    {
      public:
        Region() = default;
        Region(Pointer phys, Pointer virt, usize size,
               i32 flags = PROT_READ | PROT_WRITE, FileDescriptor* fd = nullptr)
            : m_VirtualSpace(virt, size)
            , m_PhysicalBase(phys)
            , m_Flags(flags)
            , m_Fd(fd)

        {
        }

        inline bool Contains(const Pointer address) const
        {
            return m_VirtualSpace.Contains(address);
        }

        inline const AddressRange& GetVirtualRange() const
        {
            return m_VirtualSpace;
        }
        inline Pointer GetPhysicalBase() const { return m_PhysicalBase; }
        inline Pointer GetVirtualBase() const
        {
            return m_VirtualSpace.GetBase();
        }
        inline usize GetSize() const { return m_VirtualSpace.GetSize(); }
        inline FileDescriptor* GetFileDescriptor() const { return m_Fd; }

        constexpr bool         ValidateFlags(i32 flags) const
        {
            return m_Flags & flags;
        }
        constexpr bool IsReadable() const { return m_Flags & PROT_READ; }
        constexpr bool IsWriteable() const { return m_Flags & PROT_WRITE; }
        constexpr bool IsExecutable() const { return m_Flags & PROT_EXEC; }

      private:
        AddressRange    m_VirtualSpace;
        Pointer         m_PhysicalBase = nullptr;
        i32             m_Flags        = 0;
        FileDescriptor* m_Fd           = nullptr;
    };

}; // namespace VirtualMemoryManager
