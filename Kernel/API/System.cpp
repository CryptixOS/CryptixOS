/*
 * Created by v1tr10l7 on 15.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/System.hpp>

#include <Arch/Arch.hpp>
#include <Arch/PowerManager.hpp>

namespace API::System
{
    ErrorOr<isize> Uname(utsname* out)
    {
        std::strncpy(out->sysname, "Cryptix", sizeof(out->sysname));
        std::strncpy(out->nodename, "cryptix", sizeof(out->sysname));
        std::strncpy(out->release, "0.0.1", sizeof(out->sysname));
        std::strncpy(out->version, __DATE__ " " __TIME__, sizeof(out->sysname));
        std::strncpy(out->machine, CTOS_ARCH_STRING, sizeof(out->sysname));

        return 0;
    }
    ErrorOr<isize> Reboot(RebootCommand cmd)
    {
        switch (cmd)
        {
            case RebootCommand::eRestart: PowerManager::Reboot(); break;
            case RebootCommand::ePowerOff: PowerManager::PowerOff(); break;

            default: Prism::Log::Warn("SysReboot: Unknown OpCode!"); break;
        }

        return Error(ENOSYS);
    }

    ErrorOr<uintptr_t> SysPanic(const char* errorMessage)
    {
        panic(std::format("SYS_PANIC: {}", errorMessage));
        return -1;
    }
} // namespace API::System
