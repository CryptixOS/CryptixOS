/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/DeviceIDs.hpp>
#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/TTY/MasterPTY.hpp>
#include <Drivers/TTY/PTY.hpp>
#include <Drivers/TTY/PTYMultiplexer.hpp>
#include <Drivers/TTY/SlavePTY.hpp>
#include <Scheduler/Process.hpp>
#include <VFS/DevPtsFs/DevPtsFs.hpp>
#include <VFS/VFS.hpp>

PTYMultiplexer* PTYMultiplexer::s_PTMX = nullptr;

PTYMultiplexer::PTYMultiplexer(DeviceMinor minor)
    : CharacterDevice("ptmx", MakeDevice(API::DeviceMajor::TTYAUX, minor))
{
}
PTYMultiplexer::~PTYMultiplexer() {}

ErrorOr<File*> PTYMultiplexer::Open(File* file)
{
    LogTrace("PTYMultiplexer: Open called");
    auto index        = TryOrRet(PTY::AllocateIndex());
    auto pair         = CreateRef<PTYPair>(index);

    auto slave        = CreateRef<SlavePTY>(pair);
    auto master       = CreateRef<MasterPTY>(pair);
    pair->Slave       = slave;
    pair->Master      = master;
    pair->MasterOpens = 1;

    DeviceManager::RegisterCharDevice(slave.Raw());
    DeviceManager::RegisterCharDevice(master.Raw());

    auto* current = Process::Current();
    auto  devpts  = DevPtsFs::Instance();
    if (!devpts
        || !devpts->AddSlave(index, slave.Raw(), current->EffectiveUserID(),
                             current->EffectiveGroupID()))
    {
        pair->MasterOpens = 0;
        pair->OnMasterClosed();
        return Error(ENOSPC);
    }

    return master.Raw();
}
