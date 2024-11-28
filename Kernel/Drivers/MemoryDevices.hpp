/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Drivers/Device.hpp"

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

#include <cerrno>

namespace MemoryDevices
{

    class Null final : public Device
    {
      public:
        Null()
            : Device(DriverType::eMemoryDevice, DeviceType::eNull)
        {
        }

        virtual std::string_view GetName() const noexcept override
        {
            return "null";
        }

        virtual isize Read(void* dest, off_t offset, usize bytes) override
        {
            return EOF;
        }
        virtual isize Write(const void* src, off_t offset, usize bytes) override
        {
            return bytes;
        }

        virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }
    };

    class Zero final : public Device
    {
      public:
        Zero()
            : Device(DriverType::eMemoryDevice, DeviceType::eZero)
        {
        }

        virtual std::string_view GetName() const noexcept override
        {
            return "zero";
        }

        virtual isize Read(void* dest, off_t offset, usize bytes) override
        {
            std::memset(dest, 0, bytes);

            return bytes;
        }
        virtual isize Write(const void* src, off_t offset, usize bytes) override
        {
            return bytes;
        }

        virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }
    };

    class Full final : public Device
    {
      public:
        Full()
            : Device(DriverType::eMemoryDevice, DeviceType::eFull)
        {
        }

        virtual std::string_view GetName() const noexcept override
        {
            return "full";
        }

        virtual isize Read(void* dest, off_t offset, usize bytes) override
        {
            std::memset(dest, 0, bytes);

            return bytes;
        }
        virtual isize Write(const void* src, off_t offset, usize bytes) override
        {
            errno = ENOSPC;
            return -1;
        }

        virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }
    };

    inline void Initialize()
    {
        Device* devices[] = {
            new Null,
            new Zero,
            new Full,
        };

        for (auto device : devices)
        {
            DevTmpFs::RegisterDevice(device);
            std::string path = "/dev/";
            path += device->GetName();

            VFS::MkNod(VFS::GetRootNode(), path, 0666, device->GetID());
        }
    }
}; // namespace MemoryDevices
