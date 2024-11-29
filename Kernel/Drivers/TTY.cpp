/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "TTY.hpp"

#include "Drivers/Terminal.hpp"
#include "VFS/DevTmpFs/DevTmpFs.hpp"
#include "VFS/VFS.hpp"

namespace
{
    std::vector<TTY*> s_TTYs;
}

TTY::TTY(Terminal* terminal, usize minor)
    : Device(DriverType::eTTY, static_cast<DeviceType>(minor))
    , terminal(terminal)
{
}

isize TTY::Read(void* dest, off_t offset, usize bytes)
{
    CtosUnused(dest);
    CtosUnused(offset);
    CtosUnused(bytes);

    return 0;
}
isize TTY::Write(const void* src, off_t offset, usize bytes)
{
    const char*      s = reinterpret_cast<const char*>(src);

    std::string_view str(s + offset, bytes);

    terminal->PrintString(str);
    return bytes;
}

i32 TTY::IoCtl(usize request, uintptr_t argp)
{
    CtosUnused(request);
    CtosUnused(argp);

    return 0;
}

void TTY::Initialize()
{
    AssertPMM_Ready();

    auto& terminals = Terminal::EnumerateTerminals();

    /*usize minor     = 1;
    for (usize i = 0; i < terminals.size(); i++)
    {
        LogTrace("TTY: Creating device /dev/tty{}...", minor);

        auto tty = new TTY(terminals[i], minor);
        s_TTYs.push_back(tty);
        DevTmpFs::RegisterDevice(tty);

        std::string path = "/dev/tty";
        path += std::to_string(minor);
        VFS::MkNod(VFS::GetRootNode(), path, 0666, tty->GetID());
        minor++;
    }

    if (!s_TTYs.empty())
        VFS::MkNod(VFS::GetRootNode(), "/dev/tty0", 0666, s_TTYs[0]->GetID());*/

    auto  tty       = new TTY(terminals[0], 0);
    s_TTYs.push_back(tty);
    DevTmpFs::RegisterDevice(tty);

    std::string path = "/dev/tty0";
    VFS::MkNod(VFS::GetRootNode(), path, 0666, tty->GetID());
    LogInfo("TTY: Initialized");
}
