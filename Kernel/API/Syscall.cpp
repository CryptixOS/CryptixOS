/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Syscall.hpp"

#include "API/VFS.hpp"

#include <array>
#include <cerrno>

namespace Syscall
{
    void Initialize();

    struct Syscall
    {
        std::string                          name;
        std::function<uintptr_t(Arguments&)> handler;

        inline uintptr_t                     operator()(Arguments& args)
        {
            if (handler.operator bool()) return handler(args);

            return 0;
        }
        inline operator bool() { return handler.operator bool(); }
    };
    static std::array<Syscall, 512> syscalls;

    void                            RegisterHandler(usize                                     index,
                                                    std::function<uintptr_t(Arguments& args)> handler,
                                                    std::string                               name)
    {
        syscalls[index] = {name, handler};
    }

    void InstallAll()
    {
        Initialize();
        RegisterSyscall(ID::eRead, VFS::SysRead);
        RegisterSyscall(ID::eWrite, VFS::SysWrite);
        RegisterSyscall(ID::eOpen, VFS::SysOpen);
    }
    void Handle(Arguments& args)
    {
        if (args.index >= 512 || !syscalls[args.index])
        {
            args.returnValue = -1;
            errno            = ENOSYS;
            LogError("Undefined syscall: {}", args.index);
            return;
        }

        args.returnValue = syscalls[args.index](args);
    }
} // namespace Syscall
