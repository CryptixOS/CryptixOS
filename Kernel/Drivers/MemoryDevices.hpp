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
            : Device(1, 0)
        {
        }

        virtual StringView GetName() const noexcept override { return "null"; }

        virtual ErrorOr<isize> Read(void* dest, off_t offset,
                                    usize bytes) override
        {
            return EOF;
        }
        virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                    isize offset = -1) override
        {
            return EOF;
        }
        virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                     usize bytes) override
        {
            return bytes;
        }
        virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                     isize offset = -1) override
        {
            return count;
        }

        virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }
    };

    class Zero final : public Device
    {
      public:
        Zero()
            : Device(1, 5)
        {
        }

        virtual StringView GetName() const noexcept override { return "zero"; }

        virtual ErrorOr<isize> Read(void* dest, off_t offset,
                                    usize bytes) override
        {
            std::memset(dest, 0, bytes);

            return bytes;
        }
        virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                    isize offset = -1) override
        {
            std::memset(out.Raw(), 0, count);
            return EOF;
        }
        virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                     usize bytes) override
        {
            return bytes;
        }
        virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                     isize offset = -1) override
        {
            return count;
        }

        virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }
    };

    class Full final : public Device
    {
      public:
        Full()
            : Device(1, 7)
        {
        }

        virtual StringView GetName() const noexcept override { return "full"; }

        virtual ErrorOr<isize> Read(void* dest, off_t offset,
                                    usize bytes) override
        {
            std::memset(dest, 0, bytes);

            return bytes;
        }
        virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                    isize offset = -1) override
        {
            std::memset(out.Raw(), 0, count);

            return count;
        }
        virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                     usize bytes) override
        {
            errno = ENOSPC;
            return -1;
        }
        virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                     isize offset = -1) override
        {
            return Error(ENOSPC);
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
            Path path(fmt::format("/dev/{}", device->GetName()).data());

            VFS::MkNod(VFS::GetRootNode(), PathView(path.View()), 0666,
                       device->GetID());
        }
    }
}; // namespace MemoryDevices
