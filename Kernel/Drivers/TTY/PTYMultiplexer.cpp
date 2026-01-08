/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/DeviceIDs.hpp>
#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/TTY/PTYMultiplexer.hpp>
#include <VFS/VFS.hpp>

PTYMultiplexer* PTYMultiplexer::s_PTMX = nullptr;

PTYMultiplexer::PTYMultiplexer(DeviceMinor minor)
    : CharacterDevice("ptmx", MakeDevice(API::DeviceMajor::TTYAUX, minor))
{
}
PTYMultiplexer::~PTYMultiplexer() {}

ErrorOr<void> PTYMultiplexer::Initialize()
{
    s_PTMX = new PTYMultiplexer();
    if (auto result = DeviceManager::RegisterCharDevice(s_PTMX); !result)
        return Error(result.Error());
    if (auto result
        = VFS::CreateNode("/dev/ptmx", S_IFCHR | 0666, s_PTMX->ID());
        !result)
        return Error(result.Error());

    return {};
}

static INodeID NewPTSIndex()
{
    static Atomic<usize> s_NextID = 0;
    return ++s_NextID;
}
ErrorOr<File*> PTYMultiplexer::Open(File* file)
{
    auto newIndex = NewPTSIndex();
    IgnoreUnused(newIndex);
    // TODO(v1tr10l7): Open new pty master/slave pair
    return Error(ENOSYS);
}
