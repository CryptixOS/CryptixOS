/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <API/Posix/fcntl.h>
#include <API/UnixTypes.hpp>
#include <API/VFS.hpp>

#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/Path.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

namespace Syscall::VFS
{
    using Syscall::Arguments;
    using namespace ::VFS;

    isize SysRead(Arguments& args)
    {
        i32      fd      = args.Args[0];
        void*    buffer  = reinterpret_cast<void*>(args.Args[1]);
        usize    bytes   = args.Args[2];

        Process* current = CPU::GetCurrentThread()->parent;
        auto     file    = current->GetFileHandle(fd);

        file->Lock();
        isize bytesRead = file->node->Read(buffer, file->offset, bytes);
        file->offset += bytesRead;
        file->Unlock();
        return bytesRead;
    }
    isize SysWrite(Arguments& args)
    {
        i32         fd      = args.Args[0];
        const char* message = reinterpret_cast<const char*>(args.Args[1]);
        usize       length  = args.Args[2];

        if (!message || length <= 0)
        {
            LogError("Invalid pointer or size");
            return 1;
        }
        Process* current = CPU::GetCurrentThread()->parent;
        auto     file    = current->GetFileHandle(fd);
        if (!file) return_err(-1, EBADF);

        std::string_view str(message, length);
        return file->node->Write(message, 0, length);
    }

    i32 SysOpen(Arguments& args)
    {
        Process* current = CPU::GetCurrentThread()->parent;
        PathView path    = reinterpret_cast<const char*>(args.Args[0]);
        i32      flags   = static_cast<i32>(args.Args[1]);
        mode_t   mode    = static_cast<mode_t>(args.Args[2]);

        return current->OpenAt(AT_FDCWD, path, flags, mode);
    }

    off_t SysLSeek(Syscall::Arguments& args)
    {
        i32             fd       = args.Args[0];
        off_t           offset   = args.Args[1];
        i32             whence   = args.Args[2];

        Process*        current  = CPU::GetCurrentThread()->parent;
        auto            file     = current->GetFileHandle(fd);

        constexpr usize SEEK_SET = 0;
        constexpr usize SEEK_CUR = 1;
        constexpr usize SEEK_END = 2;

        switch (whence)
        {
            case SEEK_SET: file->offset = offset; break;
            case SEEK_CUR:
                if (usize(file->offset) + usize(offset) > sizeof(off_t))
                    return_err(-1, EOVERFLOW);
                file->offset += offset;
                break;
            case SEEK_END:
            {
                usize size = file->node->GetStats().st_size;
                if (usize(file->offset) + size > sizeof(off_t))
                    return_err(-1, EOVERFLOW);
                file->offset = file->node->GetStats().st_size + offset;
                break;
            }

            default: return_err(-1, EINVAL);
        };

        return file->offset;
    }

    int SysIoCtl(Syscall::Arguments& args)
    {
        int           fd      = args.Args[0];
        unsigned long request = args.Args[1];
        usize         arg     = args.Args[2];

        Process*      current = CPU::GetCurrentThread()->parent;
        auto          file    = current->GetFileHandle(fd);
        if (!file) return_err(-1, EBADF);

        return file->node->IoCtl(request, arg);
    }

    i32 SysOpenAt(Syscall::Arguments& args)
    {
        Process* current = CPU::GetCurrentThread()->parent;

        i32      dirFd   = static_cast<i32>(args.Args[0]);
        PathView path    = reinterpret_cast<const char*>(args.Args[1]);
        i32      flags   = static_cast<i32>(args.Args[2]);
        mode_t   mode    = static_cast<mode_t>(args.Args[3]);

        return current->OpenAt(dirFd, path, flags, mode);
    }
}; // namespace Syscall::VFS
