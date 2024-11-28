/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "VFS.hpp"

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
        if (fd != 2)
        {
            LogError("Invalid file descriptor");
            return 1;
        }

        std::string_view str(message, length);
        Logger::Log(LogLevel::eNone, str.data());

        return 0;
    }
}; // namespace VFS
