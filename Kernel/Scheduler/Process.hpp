/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <vector>

using pid_t = i64;
using tid_t = i64;

enum class PrivilegeLevel
{
    ePrivileged,
    eUnprivileged,
};

struct Credentials
{
    uid_t uid;
    gid_t gid;
    uid_t euid;
    gid_t egid;
    uid_t suid;
    gid_t sgid;
    pid_t sid;
    gid_t pgid;
};

constexpr Credentials ROOT_CREDENTIALS = {
    .uid  = 0,
    .gid  = 0,
    .euid = 0,
    .egid = 0,
    .suid = 0,
    .sgid = 0,
    .sid  = 0,
    .pgid = 0,
};

struct Thread;

struct Process
{
    Process() = default;
    Process(std::string_view name, PrivilegeLevel ring);

    Process* Fork();

    void     InitializeStreams()
    {
        fileDescriptors.clear();
        INode* currentTTY
            = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), "/dev/tty0"));

        LogTrace("Process: Creating standard streams...");
        fileDescriptors.push_back(currentTTY->Open());
        fileDescriptors.push_back(currentTTY->Open());
        fileDescriptors.push_back(currentTTY->Open());
    }
    i32                          Exit(i32 code);

    pid_t                        pid         = -1;
    std::string                  name        = "?";
    PageMap*                     pageMap     = nullptr;
    std::atomic<tid_t>           nextTid     = 1;
    Credentials                  credentials = {};
    PrivilegeLevel               ring        = PrivilegeLevel::eUnprivileged;

    Process*                     parent      = nullptr;
    std::vector<Process*>        m_Children;
    std::vector<Thread*>         threads;

    std::vector<FileDescriptor*> fileDescriptors;
    std::vector<VMM::Region>     m_AddressSpace{};
    uintptr_t                    userStackTop = 0x70000000000;
};
