/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "VFS.hpp"

#include "Arch/CPU.hpp"

#include "API/UnixTypes.hpp"
#include "Scheduler/Process.hpp"
#include "Scheduler/Thread.hpp"

#include "VFS/FileDescriptor.hpp"
#include "VFS/INode.hpp"
#include "VFS/VFS.hpp"

namespace VFS
{
    using Syscall::Arguments;
    isize SysWrite(Arguments& args)
    {
        i32         fd      = args.args[0];
        const char* message = reinterpret_cast<const char*>(args.args[1]);
        usize       length  = args.args[2];

        if (!message || length <= 0)
        {
            LogError("Invalid pointer or size");
            return 1;
        }
        Process* current = CPU::GetCurrentThread()->parent;
        if (fd >= static_cast<i32>(current->fileDescriptors.size()))
        {
            LogError("Invalid fd");
            return EBADF;
        }
        auto             file = current->fileDescriptors[fd];

        std::string_view str(message, length);
        return file->node->Write(message, 0, length);
    }

    i32 SysOpen(Arguments& args)
    {
        Process*    current = CPU::GetCurrentThread()->parent;

        const char* path    = reinterpret_cast<const char*>(args.args[0]);
        auto node = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), path));
        Assert(node);
        auto descriptor = node->Open();

        if (!descriptor) return ENOSYS;

        usize fd = current->fileDescriptors.size();
        current->fileDescriptors.push_back(descriptor);

        return fd;
    }

    isize SysRead(Arguments& args)
    {
        i32   fd     = args.args[0];
        void* buffer = reinterpret_cast<void*>(args.args[1]);
        usize bytes  = args.args[2];

        LogInfo("Reading {} bytes from fd[{}] to address: {:#x}", bytes, fd,
                u64(buffer));

        Process* current = CPU::GetCurrentThread()->parent;
        if (fd >= static_cast<i32>(current->fileDescriptors.size()))
            return EBADF;
        auto file = current->fileDescriptors[fd];

        return file->node->Read(buffer, 0, bytes);
    }
}; // namespace VFS
