/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/UnixTypes.hpp>
#include <Library/UserBuffer.hpp>

#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Utility/Optional.hpp>
#include <VFS/File.hpp>

using DeviceMajor = u32;
using DeviceMinor = u32;

constexpr dev_t MakeDevice(DeviceMajor major, DeviceMinor minor)
{
    dev_t dev = dev_t(major & 0x00000fffu) << 8;
    dev |= dev_t(major & 0xfffff000u) << 32;
    dev |= dev_t(minor & 0x000000ffu);
    dev |= dev_t(minor & 0xffffff00u) << 12;

    return dev;
}

class Device : public File
{
  public:
    constexpr Device(StringView name, dev_t id)
        : m_Name(name)
        , m_ID(id)
    {
    }
    Device(DeviceMajor major, DeviceMinor minor)
        : m_ID(MakeDevice(major, minor))
    {
    }
    Device(DeviceMajor major)
        : Device(major, AllocateMinor())
    {
    }

    constexpr inline dev_t ID() const noexcept { return m_ID; }
    virtual StringView     Name() const noexcept = 0;

    virtual const stat&    Stats() { return m_Stats; }

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1)
        = 0;
    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) = 0;
    virtual ErrorOr<isize> Write(const void* src, off_t offset, usize bytes)
        = 0;
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1)
        = 0;

    virtual i32 IoCtl(usize request, uintptr_t argp) = 0;

    static void Initialize();

    Device*     Next() const { return Hook.Next; }
    using List = IntrusiveList<Device>;

  protected:
    StringView                   m_Name = ""_sv;
    dev_t                        m_ID;
    stat                         m_Stats;

    static Optional<DeviceMajor> s_LeastMajor;
    static Bitmap                s_AllocatedMajors;

    static Optional<DeviceMajor> AllocateMajor(usize hint = 0);
    static void                  FreeMajor(DeviceMajor major);

  private:
    friend class IntrusiveList<Device>;
    friend struct IntrusiveListHook<Device>;

    IntrusiveListHook<Device> Hook;

    static DeviceMinor        AllocateMinor()
    {
        // TODO(v1tr10l7): Allocate minor numbers per major
        static Atomic<DeviceMinor> s_Base = 1000;

        return s_Base++;
    }

    static Optional<DeviceMajor> FindFreeMajor(isize start, isize end);
    static Optional<DeviceMajor> FindFreeMajor(DeviceMajor hint);
};
