/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/DeviceIDs.hpp>
#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/TTY/MasterPTY.hpp>
#include <Drivers/TTY/PTYMultiplexer.hpp>
#include <VFS/VFS.hpp>

PTYMultiplexer* PTYMultiplexer::s_PTMX = nullptr;

PTYMultiplexer::PTYMultiplexer(DeviceMinor minor)
    : CharacterDevice("ptmx", MakeDevice(API::DeviceMajor::TTYAUX, minor))
{
}
PTYMultiplexer::~PTYMultiplexer() {}

static INodeID NewPTSIndex()
{
    static Atomic<usize> s_NextID = 0;
    return ++s_NextID;
}
ErrorOr<File*> PTYMultiplexer::Open(File* file)
{
    LogTrace("PTYMultiplexer: Open called");
    auto   newIndex = NewPTSIndex();
    String ptyName  = fmt::format("pts{}", newIndex).data();
    auto   master   = new MasterPTY(ptyName, newIndex);

    DeviceManager::RegisterCharDevice(master);
    auto ptyPath = fmt::format("/dev/pts/{}", ptyName).data();
    VFS::CreateNode(ptyPath, S_IFCHR | 0666, master->ID());

    // TODO(v1tr10l7): Open new pty master/slave pair
    return master;
}
